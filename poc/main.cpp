#include <QImage>
#include <QRect>
#include <QPainter>
#include <QGuiApplication>
#include <cstdio>
#include <QPaintEngine>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdint.h>

using namespace std;

class FBStuff {
    public:
        FBStuff(){
            instance = getInstance();
        }

        void DrawText(char *text) {
              QRect rect(100,100,200,100);
              QPainter painter((QPaintDevice*)(instance+8));
              printf("after ctor\n");
              painter.drawText(rect, 132,text);
              painter.end();
              printf("Sending update\n");
              sendUpdate(0, rect, 0,10);
        }


    private:
        uint32_t* instance;

        //black magic
        uint32_t* (*getInstance)(void) = (uint32_t*(*)(void)) 0x224BC;
        void (*sendUpdate)(void*, ...) = (void (*)(void*,...)) 0x2257C;
};


extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() { printf("LIBHOOK INIT\n"); }

int main(int argc, char **argv, char **envp) {
  qputenv("QMLSCENE_DEVICE", "epaper");
  qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");
  //needed for qpainter
  QGuiApplication app(argc, argv);
  printf("OUR MAIN\n");
  //auto instance = getInstance();
  printf("got instance\n");
  printf("getting painter\n");

  FBStuff stuff;
  stuff.DrawText("Testing");

  //QImage or something

  //first param is ignored,rect, waveform, ?
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
