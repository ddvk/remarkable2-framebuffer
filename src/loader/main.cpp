#include "../shared/swtfb.cpp"

#include <cstdio>
#include <iostream>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <stdint.h>


extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() { printf("LIBHOOK INIT\n"); }

int main(int, char **, char **) {
  swtfb::SwtFB fb;
  fb.FullScreen();
  fb.DrawLine();
  for (int i = 0; i < 1000; i += 50) {
    fb.DrawText(i, "Testing", false);
  }
  fb.DrawText(1800, "Done", true);

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
