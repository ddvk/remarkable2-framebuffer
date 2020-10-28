#include <QImage>
#include <QGuiApplication>
#include <QPaintEngine>
#include <QPainter>
#include <QRect>
#include <chrono>
#include <cstdio>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdint.h>

#include "../shared/ipc.cpp"

int msg_q_id = 0x2257c;
ipc::Queue MSGQ(msg_q_id);

using namespace std;
const int maxWidth = 1404;
const int maxHeight = 1872;

class ClockWatch {
public:
  chrono::high_resolution_clock::time_point t1;

  ClockWatch() { t1 = chrono::high_resolution_clock::now(); }

  auto elapsed() {
    auto t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> time_span =
        chrono::duration_cast<chrono::duration<double>>(t2 - t1);
    return time_span.count();
  }
};

// todo: make it singleton
class SwtFB {
public:
  SwtFB() {
    qputenv("QMLSCENE_DEVICE", "epaper");
    qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");

    // needed for qpainter
    char *argv[0];
    int argc = 0;
    app = new QGuiApplication(argc, argv);
    instance = getInstance();
    img = (QImage *)(instance + 8);
    printf("INSTANCE ADDRESS: 0x%lx\n", instance);

    cout << img->width() << " " << img->height() << " " << img->depth() << endl;
  }

  void DrawLine() {

      for(int i=0; i < maxHeight; i++) {
          img->setPixel(100, i, 0xF0);
          QRect rect(99, i+1, 2, 2);
          sendUpdate(0, rect, 1, 4); 
          printf(".");
      }
  }

  void DrawText(int i, char *text, bool wait) {
    QRect rect(100, i, 200, 100);
    QPainter painter(img);
    painter.drawText(rect, 132, text);
    painter.end();
    int w = wait ? 2 : 0;
    //sendUpdate(0, rect, 3, 3); //slow
    //sendUpdate(0, rect, 3, 2); //slow but nice
    //sendUpdate(0, rect, 2, 1); //flashing background
    //sendUpdate(0, rect, 2, 0); //flashing background
    sendUpdate(0, rect, 2, 4); 
  }

  void DrawRaw(uint16_t *buffer, int x, int y, int w, int h) {
    uint16_t* dest = (uint16_t*) img->bits();

    int stride = maxWidth;
    int x0 = x, y0 = y, x1 = x0 + w, y1 = y0 + h;

    cout << "CORNERS" << " " << x0 << " " << y0 << " " << x1 << " " << y1 << endl;

    for (int i = y0; i < y1; i++) {
      memcpy(&dest[i * stride + x0], &buffer[i * stride + x0],
             (x1 - x0) * sizeof(uint16_t));
    }

    QRect rect(x, y, w, h);
    ClockWatch cz;
    sendUpdate(0, rect, 3, 2);
    cout << "Update took " << cz.elapsed() << "s" << endl;
  }

private:
  uint32_t *instance;
  QGuiApplication *app;
  QImage *img;

  // black magic
  uint32_t *(*getInstance)(void) = (uint32_t * (*)(void))0x224BC;
  void (*sendUpdate)(void *, ...) = (void (*)(void *, ...))0x2257C;
};

extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() { printf("LIBHOOK INIT\n"); }

int main(int argc, char **argv, char **envp) {
  SwtFB fb;
  fb.DrawLine();
  for (int i = 0; i < 1000; i += 50) {
    fb.DrawText(i, "Testing", false);
  }
  fb.DrawText(1800, "Done", true);

  uint16_t *shared_mem = ipc::get_shared_buffer();

  printf("WAITING FOR SEND UPDATE ON MSG Q");
  while (true) {
    ipc::msg_rect buf = MSGQ.recv();
    fb.DrawRaw(shared_mem, buf.x, buf.y, buf.w, buf.h);

#ifdef DEBUG_MSGQ
    for (int i = 0; i < 10; i++) {
      printf("%i, ", shared_mem[i]);
    }
    printf("\n");
    memset(shared_mem, 0, 100);
#endif
  }
  printf("END of our main\n");
}

int __libc_start_main(int (*_main)(int, char **, char **), int argc,
                      char **argv, int (*init)(int, char **, char **),
                      void (*fini)(void), void (*rtld_fini)(void),
                      void *stack_end) {

  printf("LIBC START HOOK\n");

  typeof(&__libc_start_main) func_main =
      (typeof(&__libc_start_main))dlsym(RTLD_NEXT, "__libc_start_main");

  return func_main(main, argc, argv, init, fini, rtld_fini, stack_end);
};
};
