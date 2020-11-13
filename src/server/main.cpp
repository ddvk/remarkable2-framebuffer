#include "../shared/ipc.cpp"
#include "../shared/swtfb.cpp"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <dlfcn.h>
#include <mutex>
#include <stdint.h>
#include <thread>

using namespace std;
using namespace swtfb;

int msg_q_id = 0x2257c;
ipc::Queue MSGQ(msg_q_id);

const int SIZE = 2;

uint16_t *shared_mem;

extern "C" {
// QImage(width, height, format)
static void (*qImageCtor)(void *that, int x, int y, int f) = 0;
// QImage(uchar*, width, height, bytesperline, format)
static void (*qImageCtorWithBuffer)(void *that, uint8_t *, int32_t x, int32_t y,
                                    int32_t bytes, int format, void (*)(void *),
                                    void *) = 0;
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() {
  shared_mem = ipc::get_shared_buffer();

  qImageCtor = (void (*)(void *, int, int, int))dlsym(
      RTLD_NEXT, "_ZN6QImageC1EiiNS_6FormatE");
  qImageCtorWithBuffer = (void (*)(
      void *, uint8_t *, int32_t, int32_t, int32_t, int, void (*)(void *),
      void *))dlsym(RTLD_NEXT, "_ZN6QImageC1EPhiiiNS_6FormatEPFvPvES2_");
}

bool FIRST_ALLOC = true;
void _ZN6QImageC1EiiNS_6FormatE(void *that, int x, int y, int f) {

  if (x == WIDTH && y == HEIGHT && FIRST_ALLOC) {
    fprintf(stderr, "REPLACING THE IMAGE with /dev/shm/xofb \n");

    FIRST_ALLOC = false;
    qImageCtorWithBuffer(that, (uint8_t *)shared_mem, WIDTH, HEIGHT,
                         WIDTH * SIZE, f, nullptr, nullptr);
    return;
  }
  qImageCtor(that, x, y, f);
}

void doUpdate(const SwtFB &fb, const swtfb_update &s) {
  auto mxcfb_update = s.update;
  auto rect = mxcfb_update.update_region;
#ifdef DEBUG_DIRTY
  std::cerr << "Dirty Region: " << rect.left << " " << rect.top << " "
            << rect.width << " " << rect.height << endl;
#endif

  int waveform = mxcfb_update.waveform_mode;
  // full = 1, partial = 0
  auto update_mode = mxcfb_update.update_mode;

  // For now, we are using two waveform modes:
  // 1: DU - direct update, fast
  // 2: GC16 - high fidelity (slow)
  // 3: GL16 - what the rm is using
  if (waveform > 3) {
    waveform = 3;
  }
  auto flags = 0;

  if (waveform == 1 && update_mode == 0) {
    // fast draw
    flags = 4;
  } else if (update_mode == 1 && waveform == 2) {
    // blink
    flags = 3;
  }
#ifdef DEBUG
  cerr << "doUpdate " << endl;
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

int server_main(int, char **argv, char **) {
  swtfb::SDK_BIN = argv[0];
  SwtFB fb;

  printf("WAITING FOR SEND UPDATE ON MSG Q\n");
  while (true) {
    swtfb_update buf = MSGQ.recv();
    doUpdate(fb, buf);
  }
}

int __libc_start_main(int (*_main)(int, char **, char **), int argc,
                      char **argv, int (*init)(int, char **, char **),
                      void (*fini)(void), void (*rtld_fini)(void),
                      void *stack_end) {

  printf("STARTING RM2FB\n");

  typeof(&__libc_start_main) func_main =
      (typeof(&__libc_start_main))dlsym(RTLD_NEXT, "__libc_start_main");

  return func_main(server_main, argc, argv, init, fini, rtld_fini, stack_end);
};
};
