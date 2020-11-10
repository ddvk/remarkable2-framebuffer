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

extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() {}

int server_main(int, char **argv, char **) {
  swtfb::SDK_BIN = argv[0];
  SwtFB fb;
  uint16_t *shared_mem = ipc::get_shared_buffer();


  mutex draw_queue_m;
  vector<swtfb_update> updates;

  auto th = new thread([&]() {
    while (true) {
      mxcfb_rect dirty_area;
      swtfb::reset_dirty(dirty_area);

      draw_queue_m.lock();
			auto todo = updates;
      updates.clear();
      draw_queue_m.unlock();

      for (auto s : todo) {
        auto mxcfb_update = s.update;
        auto rect = mxcfb_update.update_region;
        #ifdef DEBUG_DIRTY
				std::cerr << "Dirty Region: " << rect.left << " " << rect.top << " "
						 << rect.width << " " << rect.height << endl;
        #endif

        int mode = mxcfb_update.waveform_mode;

        // For now, we are using two waveform modes:
        // 1: DU - direct update, fast
        // 2: GC16 - high fidelity (slow)
        //
        if (mxcfb_update.waveform_mode > 2) {
          mode = 2;
        }

				int size = rect.width * rect.height;
        fb.DrawRaw(shared_mem, rect.left, rect.top, rect.width, rect.height,
            mode, 0);

#ifdef DEBUG_TIMING
        cerr << get_now() - s.ms << "ms E2E " << rect.width << " " << rect.height << endl;
#endif

      }
      usleep(1000);

    }

  });

  printf("WAITING FOR SEND UPDATE ON MSG Q\n");
  while (true) {
    swtfb_update buf = MSGQ.recv();

    draw_queue_m.lock();
    updates.push_back(buf);
    draw_queue_m.unlock();
  }
  th->join();
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
