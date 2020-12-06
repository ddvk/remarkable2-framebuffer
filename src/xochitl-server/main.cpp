
#include <fcntl.h>

#include "../shared/ipc.cpp"
#include "../shared/swtfb.cpp"

#include <linux/input.h>
#include <sys/poll.h>
#include <unistd.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <dlfcn.h>
#include <mutex>
#include <stdint.h>
#include <thread>

#include <QCoreApplication>
#include <QTouchEvent>

// using namespace std;
using namespace swtfb;

int msg_q_id = 0x2257c;
ipc::Queue MSGQ(msg_q_id);

const int SIZE = 2;

uint16_t *shared_mem;

char backup[ipc::BUF_SIZE];

static bool ENABLED = false;

void doUpdate(const SwtFB &fb, const swtfb_update &s) {
  auto mxcfb_update = s.update;
  auto rect = mxcfb_update.update_region;

#ifdef DEBUG_DIRTY
  std::cerr << "Dirty Region: " << rect.left << " " << rect.top << " "
            << rect.width << " " << rect.height << endl;
#endif

  // For now, we are using two waveform modes:
  // 0: init (same as GL16)
  // 1: DU - direct update, fast
  // 2: GC16 - high fidelity (slow)
  // 3: GL16 - what the rm is using
  // 8: highlight (same as high fidelity)

  int waveform = mxcfb_update.waveform_mode;
  // full = 1, partial = 0
  auto update_mode = mxcfb_update.update_mode;

  auto flags = update_mode & 0x1;

  // TODO: Get sync from client (wait for second ioctl? or look at stack?)
  // There are only two occasions when the original rm1 library sets sync to
  // true. Currently we detect them by the other data. Ideally we should
  // correctly handle the corresponding ioctl (empty rect and flags == 2?).
  if (waveform == /*init*/ 0 && update_mode == /* full */ 1) {
    flags |= 2;
    std::cerr << "SERVER: sync" << std::endl;
  } else if (rect.left == 0 && rect.top > 1800 &&
             waveform == /* grayscale */ 3 && update_mode == /* full */ 1) {
    std::cerr << "server sync, x2: " << rect.width << " y2: " << rect.height
              << std::endl;
    flags |= 2;
  }

#ifdef DEBUG
  std::cerr << "doUpdate " << endl;
  cerr << "mxc: waveform_mode " << mxcfb_update.waveform_mode << endl;
  cerr << "mxc: update mode " << mxcfb_update.update_mode << endl;
  cerr << "mxc: update marker " << mxcfb_update.update_marker << endl;
  cerr << "final: waveform " << waveform;
  cerr << " flags " << flags << endl << endl;
#endif

  fb.DrawRaw(rect.left, rect.top, rect.width, rect.height, waveform, flags);

#ifdef DEBUG_TIMING
  cerr << get_now() -.ms << "ms E2E " << rect.width << " " << rect.height
       << endl;
#endif
}

class TestFilter : public QObject {
  Q_OBJECT;

public:
  TestFilter() : QObject(nullptr) {}

  static void updateLoop() {
    static SwtFB fb;

    // Backup the screen
    memcpy(backup, shared_mem, ipc::BUF_SIZE);
    // TODO: what is needed here?
    fb.ClearScreen();
    // fb.ClearGhosting();

    std::thread updateThread([]() {
      system("LD_PRELOAD=/home/root/librm2fb_client.so.1.0.0 /home/root/draft");

      // MSGQ.send({}, /* stop */ true);
      fprintf(stderr, "Process stopped\n");
    });

    while (true) {
      auto buf = MSGQ.recv();
      // if (buf.mtype == ipc::STOP_t) {
      //   break;
      // }

      doUpdate(fb, buf);
    }

    fprintf(stderr, "joining");
    updateThread.join();

    fprintf(stderr, "stopped, restoring screen\n");

    // fb.ClearScreen();
    memcpy(shared_mem, backup, ipc::BUF_SIZE);
    fb.DrawRaw(0, 0, ipc::maxWidth, ipc::maxHeight, /* grayscale TODO: hq? */ 3,
               /* sync and full */ 3);

    fprintf(stderr, "Done!\n");
  }

  bool eventFilter(QObject *object, QEvent *event) override {
    auto type = event->type();
    if (type == QEvent::TouchBegin) {
      auto *touchEv = static_cast<QTouchEvent *>(event);
      auto points = touchEv->touchPoints().size();

      if (points == 2) {
        updateLoop();
        return true;
      }
    }

    return false;
  }

private:
};

extern "C" {
// QImage(width, height, format)
static void (*qImageCtor)(void *that, int x, int y, int f) = 0;
// QImage(uchar*, width, height, bytesperline, format)
static void (*qImageCtorWithBuffer)(void *that, uint8_t *, int32_t x, int32_t y,
                                    int32_t bytes, int format, void (*)(void *),
                                    void *) = 0;

static int (*safe_poll)(struct pollfd *fds, nfds_t nfds,
                        const struct timespec *timeout_ts) = 0;

static int (*qcoreAppExec)();

static void _libhook_init() __attribute__((constructor));
static void _libhook_init() {
  shared_mem = ipc::get_shared_buffer();

  qImageCtor = (void (*)(void *, int, int, int))dlsym(
      RTLD_NEXT, "_ZN6QImageC1EiiNS_6FormatE");
  qImageCtorWithBuffer = (void (*)(
      void *, uint8_t *, int32_t, int32_t, int32_t, int, void (*)(void *),
      void *))dlsym(RTLD_NEXT, "_ZN6QImageC1EPhiiiNS_6FormatEPFvPvES2_");

  safe_poll = (int (*)(struct pollfd * fds, nfds_t nfds,
                       const struct timespec *timeout_ts))
      dlsym(RTLD_NEXT, "_Z12qt_safe_pollP6pollfdmPK8timespec");

  qcoreAppExec =
      (typeof(qcoreAppExec))dlsym(RTLD_NEXT, "_ZN15QGuiApplication4execEv");
  fprintf(stderr, "Got methods: %x\n", qcoreAppExec);
}

bool FIRST_ALLOC = true;
void _ZN6QImageC1EiiNS_6FormatE(void *that, int x, int y, int f) {
  if (ENABLED && x == WIDTH && y == HEIGHT && FIRST_ALLOC) {
    fprintf(stderr, "REPLACING THE IMAGE with /dev/shm/xofb \n");

    FIRST_ALLOC = false;
    qImageCtorWithBuffer(that, (uint8_t *)shared_mem, WIDTH, HEIGHT,
                         WIDTH * SIZE, f, nullptr, nullptr);
    return;
  }
  qImageCtor(that, x, y, f);
}

static int i = 0;

// int _Z12qt_safe_pollP6pollfdmPK8timespec(struct pollfd *fds, nfds_t nfds,
//                                          const struct timespec *timeout_ts) {
//   if (ENABLED) {
//     if (i < 10) {
//       fprintf(stderr, "POLL CALLED %d: %d\n", i, nfds);
//     }
//
//     static SwtFB fb;
//
//     swtfb_update buf;
//     if (MSGQ.poll(&buf)) {
//       doUpdate(fb, buf);
//     }
//
//     i++;
//   }
//
//   return safe_poll(fds, nfds, timeout_ts);
//   // return 0;
// }

int _ZN15QGuiApplication4execEv() {
  if (ENABLED) {
    fprintf(stderr, "\nHOOKED APP EXEC!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n\n");

    auto *app = QCoreApplication::instance();
    fprintf(stderr, "app instance: %x\n", app);

    TestFilter *filter = new TestFilter();
    app->installEventFilter(filter);
  }

  return qcoreAppExec();
}

int server_main(int, char **argv, char **) {

  printf("WAITING FOR SEND UPDATE ON MSG Q\n");
  while (true) {
  }
}

int __libc_start_main(int (*_main)(int, char **, char **), int argc,
                      char **argv, int (*init)(int, char **, char **),
                      void (*fini)(void), void (*rtld_fini)(void),
                      void *stack_end) {

  printf("STARTING RM2FB\n");

  typeof(&__libc_start_main) func_main =
      (typeof(&__libc_start_main))dlsym(RTLD_NEXT, "__libc_start_main");

  swtfb::SDK_BIN = argv[0];
  if (swtfb::SDK_BIN.find("xochitl") != std::string::npos) {
    ENABLED = true;
    fprintf(stderr, "Enabled!\n");
  }

  fprintf(stderr, "BIN FILE: %s\n", argv[0]);

  return func_main(_main /*server_main*/, argc, argv, init, fini, rtld_fini,
                   stack_end);
};
};

#include "main.moc"
