#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <cstdio>
#include <dlfcn.h>
#include <stdint.h>

#include "../shared/ipc.cpp"
#include "../shared/swtfb.cpp"

int msg_q_id = 0x2257c;
ipc::Queue MSGQ(msg_q_id);

using namespace std;

extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() { printf("LIBHOOK INIT\n"); }

int main(int, char **, char **) {
  SwtFB fb;
  uint16_t *shared_mem = ipc::get_shared_buffer();

  printf("WAITING FOR SEND UPDATE ON MSG Q");
  while (true) {
    ipc::swtfb_update buf = MSGQ.recv();
    auto rect = buf.update_region;
    cout << rect.left << " " << rect.top << " " << rect.width << " " << rect.height << endl;
    fb.DrawRaw(shared_mem, rect.left, rect.top, rect.width, rect.height);

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
