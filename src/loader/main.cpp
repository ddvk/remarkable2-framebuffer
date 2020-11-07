#include <QImage>
#include <QGuiApplication>
#include <QPaintEngine>
#include <QPainter>
#include <QRect>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <linux/fb.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdint.h>

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
      cout << "drawing a line " << endl;
      cout << "send update" << endl;
      for(int i=100; i < maxHeight; i++) {
          QRect rect(100, i, 200, 100);
          img->setPixel(100, i, 0xFF);
          printf(".");
          sendUpdate(instance, rect,  1, 0); 
      }
  }
  void FullScreen(int x = 0) {
    QString text = "Testing " + QString::number( x);
    QRect rect(0, 0, maxWidth, maxHeight);
    auto pen = x % 2 ? 0xFF : 0xED;
    img->fill(pen);
    QPainter painter(img);
    painter.setPen(0xDD);
    painter.drawText(rect, 132, text);
    painter.end();
    //sendUpdate(0, rect, 3, 3); //slow
    //sendUpdate(0, rect, 3, 2); //slow but nice
    //sendUpdate(0, rect, 2, 1); //flashing background
    //sendUpdate(0, rect, 2, 0); //flashing background
    sendUpdate(instance, rect, 3, 2); 
    //sendUpdate(instance, rect, 3, 2); 
  }

  void DrawText(int i, char *text, bool wait) {
    QRect rect(0, i, 200, 100);
    QPainter painter(img);
    painter.drawText(rect, 132, text);
    painter.end();
    int w = wait ? 2 : 0;
    //sendUpdate(0, rect, 3, 3); //slow
    //sendUpdate(0, rect, 3, 2); //slow but nice
    //sendUpdate(0, rect, 2, 1); //flashing background
    //sendUpdate(0, rect, 2, 0); //flashing background
    sendUpdate(instance, rect,  3, 2); 
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
    sendUpdate(instance, rect, 3, 2);
    cout << "Update took " << cz.elapsed() << "s" << endl;
  }

private:
  uint32_t *instance;
  QGuiApplication *app;
  QImage *img;

  // black magic
  uint32_t *(*getInstance)(void) = (uint32_t * (*)(void)) 0x21F54;
  void (*sendUpdate)(void *, ...) = (void (*)(void *, ...)) 0x21A34;
};

extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() { printf("LIBHOOK INIT\n"); }

volatile int counter = 0;
int main(int argc, char **argv, char **envp) {
  SwtFB fb;
  int i = 0;
  while(true){
      printf("stating screen \n");
      counter = 0;
      fb.FullScreen(i++);
      printf("full screen done: %d ioctl \n", counter);
      std::cin.ignore();
  }
  fb.DrawLine();
  for (int i = 0; i < 1000; i += 50) {
    fb.DrawText(i, "Testing", false);
  }
  fb.DrawText(1800, "Done", true);
  fb.FullScreen();

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
}

int ioctl(int fd, int request, ...) {
  static int (*func)(int fd, int request, ...);
  if (!func) {
    printf("Hooking ioctl(...)\n");
    func = (int (*)(int d, int request, ...)) dlsym(RTLD_NEXT, "ioctl");
  }

  va_list args;
  va_start(args, request);
  void *p = va_arg(args, void *);
  va_end(args);

  int x = 0;
  if (fd == 10) {
    char *name;
    switch(request){
        case 0x4606:
            name = "PAN2";
            counter++;
            x = ((struct fb_var_screeninfo*)p)->yoffset;

            break;
        case 0x4601:
            name = "PAN";
            x = ((struct fb_var_screeninfo*)p)->yoffset;

            break;
        case 0x4611:
            name = "BLANK";
            break;
        default:
            name = "UNKNOWN";
            break;
    }
    x /= 1408;
    printf("ioctl(%d, 0x%x (addr: %p), %p (addr: %p) %s yoffset*1408: %d \n", fd, request, &request, p, &p, name, x);
  }
  int rc = func(fd, request, p);


  return rc;
}



}
