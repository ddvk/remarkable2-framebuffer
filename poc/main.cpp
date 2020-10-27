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
static QGuiApplication *app=nullptr;
typedef int (*func_t)(void);
typedef int* (*getInstance)(void);
typedef void (*sendUpdate)(void*, ...);
uint64_t ADDR = 0x178E8;
uint64_t getInstanceADDR = 0x224BC;
uint64_t sendUpdateADDR = 0x2257C;
extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() { printf("LIBHOOK INIT\n"); }

int main(int argc, char **argv, char **envp) {
  qputenv("QMLSCENE_DEVICE", "epaper");
  qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");

  app = new QGuiApplication(argc, argv);
  printf("OUR MAIN\n");
  auto *instance = ((getInstance)getInstanceADDR)();
  printf("got instance\n");
  QRect rect(0,0,1404,1872);
  printf("getting painter\n");
  //QPainter *painter  = (QPainter*) ((QPaintDevice*) instance+4)->paintEngine();
  //printf("result %d\n", painter);
  //painter->drawText(10,10,"tst");
  printf("Sending update\n");
  ((sendUpdate)sendUpdateADDR)(instance, rect, 2,3);
  //((func_t)ADDR)();
  printf("the other one finished");
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
