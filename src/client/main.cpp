#include <dlfcn.h>
#include <iostream>
#include <libgen.h>
#include <linux/fb.h>
#include <linux/ioctl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fstream>
#include <unistd.h>
#include <time.h>

#include <QByteArray>

#include "../shared/ipc.cpp"
#include "../shared/signature.cpp"

#include "frida/frida-gum.h"

#define SEM_WAIT_TIMEOUT 200000000 /* 200 * 1000 * 1000, e.g. 200ms */

constexpr auto msg_q_id = 0x2257c;
swtfb::ipc::Queue MSGQ(msg_q_id);

uint16_t *SHARED_BUF = nullptr;

constexpr auto BYTES_PER_PIXEL = sizeof(*SHARED_BUF);

bool IN_XOCHITL = false;
bool DO_WAIT_IOCTL = true;
bool ON_RM2 = false;

extern "C" {

__attribute__((constructor))
void init() {
  std::ios_base::Init i;

  std::ifstream device_id_file{"/sys/devices/soc0/machine"};
  std::string device_id;
  std::getline(device_id_file, device_id);

  if (device_id == "reMarkable 2.0") {
    SHARED_BUF = swtfb::ipc::get_shared_buffer();
    ON_RM2 = true;

    constexpr auto VERSION = "0.1";
    setenv("RM2FB_SHIM", VERSION, true);

    if (getenv("RM2FB_ACTIVE") != nullptr) {
        setenv("RM2FB_NESTED", "1", true);
    } else {
        setenv("RM2FB_ACTIVE", "1", true);
    }

    if (getenv("RM2FB_NO_WAIT_IOCTL") != nullptr) {
      DO_WAIT_IOCTL = false;
    }
  }
}

__attribute__((visibility("default")))
void _ZN6QImageC1EiiNS_6FormatE(void *that, int x, int y, int f) {
  static bool FIRST_ALLOC = true;
  static const auto qImageCtor = (void (*)(void *, int, int, int))dlsym(
      RTLD_NEXT, "_ZN6QImageC1EiiNS_6FormatE");
  static const auto qImageCtorWithBuffer = (void (*)(
      void *, uint8_t *, int32_t, int32_t, int32_t, int, void (*)(void *),
      void *))dlsym(RTLD_NEXT, "_ZN6QImageC1EPhiiiNS_6FormatEPFvPvES2_");

  if (ON_RM2 && IN_XOCHITL && x == swtfb::WIDTH && y == swtfb::HEIGHT && FIRST_ALLOC) {
    fprintf(stderr, "REPLACING THE IMAGE with shared memory\n");

    FIRST_ALLOC = false;
    qImageCtorWithBuffer(that, (uint8_t *)SHARED_BUF, swtfb::WIDTH,
                         swtfb::HEIGHT, swtfb::WIDTH * BYTES_PER_PIXEL, f,
                         nullptr, nullptr);
    return;
  }
  qImageCtor(that, x, y, f);
}

__attribute__((visibility("default")))
int open64(const char *pathname, int flags, mode_t mode = 0) {
  if (ON_RM2 && !IN_XOCHITL) {
    if (pathname == std::string("/dev/fb0")) {
      return swtfb::ipc::SWTFB_FD;
    }
  }

  static const auto func_open = (int (*)(const char *, int, mode_t))
    dlsym(RTLD_NEXT, "open64");

  return func_open(pathname, flags, mode);
}

__attribute__((visibility("default")))
int open(const char *pathname, int flags, mode_t mode = 0) {
  if (ON_RM2 && !IN_XOCHITL) {
    if (pathname == std::string("/dev/fb0")) {
      return swtfb::ipc::SWTFB_FD;
    }
  }

  static const auto func_open = (int (*)(const char *, int, mode_t))
    dlsym(RTLD_NEXT, "open");

  return func_open(pathname, flags, mode);
}

__attribute__((visibility("default")))
int close(int fd) {
  if (ON_RM2 && fd == swtfb::ipc::SWTFB_FD) {
    return 0;
  }

  static const auto func_close = (int (*)(int))dlsym(RTLD_NEXT, "close");

  return func_close(fd);
}

__attribute__((visibility("default")))
int ioctl(int fd, unsigned long request, char *ptr) {
  if (ON_RM2 && !IN_XOCHITL && fd == swtfb::ipc::SWTFB_FD) {
    if (request == MXCFB_SEND_UPDATE) {

      mxcfb_update_data *update = (mxcfb_update_data *)ptr;
      MSGQ.send(*update);
      return 0;
    } else if (request == MXCFB_SET_AUTO_UPDATE_MODE) {

      return 0;
    } else if (request == MXCFB_WAIT_FOR_UPDATE_COMPLETE) {
#ifdef DEBUG
      std::cerr << "CLIENT: sync" << std::endl;
#endif

      if (!DO_WAIT_IOCTL) {
        return 0;
      }

      // for wait ioctl, we drop a WAIT_t message into the queue.  the server
      // then uses that message to signal the semaphore we just opened. this
      // can take as little as 0.5ms for small updates. one difference is that
      // the ioctl now waits for all pending updates, not just the requested
      // scheduled one.
      swtfb::ClockWatch cz;
      swtfb::wait_sem_data update;
      std::string sem_name = std::string("/rm2fb.wait.");
      sem_name += std::to_string(getpid());

      memcpy(update.sem_name, sem_name.c_str(), sem_name.size());
      update.sem_name[sem_name.size()] = 0;

      MSGQ.send(update);

      sem_t *sem = sem_open(update.sem_name, O_CREAT);
      struct timespec timeout;
      if (clock_gettime(CLOCK_REALTIME, &timeout) == -1) {
        // Probably unnecessary fallback
        timeout = {0, 0};
#ifdef DEBUG
        std::cerr << "clock_gettime failed" << std::endl;
#endif
      }

      timeout.tv_nsec += SEM_WAIT_TIMEOUT;
      if (timeout.tv_nsec >= 1e9) {
        timeout.tv_nsec -= 1e9;
        timeout.tv_sec++;
      }

      sem_timedwait(sem, &timeout);

      // on linux, unlink will delete the semaphore once all processes using
      // it are closed. the idea here is that the client removes the semaphore
      // as soon as possible
      // TODO: validate this assumption
      sem_unlink(update.sem_name);

#ifdef DEBUG
      std::cerr << "FINISHED WAIT IOCTL " << cz.elapsed() << std::endl;
#endif
      return 0;
    }

    else if (request == FBIOGET_VSCREENINFO) {

      fb_var_screeninfo *screeninfo = (fb_var_screeninfo *)ptr;
      screeninfo->xres = swtfb::WIDTH;
      screeninfo->yres = swtfb::HEIGHT;
      screeninfo->grayscale = 0;
      screeninfo->bits_per_pixel = 8 * BYTES_PER_PIXEL;
      screeninfo->xres_virtual = swtfb::WIDTH;
      screeninfo->yres_virtual = swtfb::HEIGHT;

      //set to RGB565
      screeninfo->red.offset = 11;
      screeninfo->red.length = 5;
      screeninfo->green.offset = 5;
      screeninfo->green.length = 6;
      screeninfo->blue.offset = 0;
      screeninfo->blue.length = 5;
      return 0;
    }

    else if (request == FBIOPUT_VSCREENINFO) {

      return 0;
    } else if (request == FBIOGET_FSCREENINFO) {

      fb_fix_screeninfo *screeninfo = (fb_fix_screeninfo *)ptr;
      screeninfo->smem_len = swtfb::ipc::BUF_SIZE;
      screeninfo->smem_start = (unsigned long)SHARED_BUF;
      screeninfo->line_length = swtfb::WIDTH * BYTES_PER_PIXEL;
      constexpr char fb_id[] = "mxcfb";
      memcpy(screeninfo->id, fb_id, sizeof(fb_id));
      return 0;
    } else {
      std::cerr << "UNHANDLED IOCTL" << ' ' << request << std::endl;
      return 0;
    }
  }

  static auto func_ioctl =
      (int (*)(int, unsigned long request, ...))dlsym(RTLD_NEXT, "ioctl");

  return func_ioctl(fd, request, ptr);
}

__attribute__((visibility("default")))
bool _Z7qputenvPKcRK10QByteArray(const char *name, const QByteArray &val) {
  static const auto touchArgs = QByteArray("rotate=180:invertx");
  static const auto orig_fn = (bool (*)(const char *, const QByteArray &))dlsym(
      RTLD_NEXT, "_Z7qputenvPKcRK10QByteArray");

  if (ON_RM2 && strcmp(name, "QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS") == 0) {
    return orig_fn(name, touchArgs);
  }

  return orig_fn(name, val);
}


void new_update(void*, int x1, int y1, int x2, int y2, int waveform, int flags) {
#ifdef DEBUG
  std::cerr << "UPDATE HOOK CALLED" << std::endl;
  std::cerr << "x " << x1 << " " << x2 << std::endl;
  std::cerr << "y " << y1 << " " << y2 << std::endl;
  std::cerr << "wav " << waveform << " flags " << flags << std::endl;
#endif

  swtfb::xochitl_data data;
  data.x1 = x1;
  data.x2 = x2;
  data.y1 = y1;
  data.y2 = y2;
  data.waveform = waveform;
  data.flags = flags;
  MSGQ.send(data);
}

int new_create_threads(char*, void*) {
  std::cerr << "create threads called" << std::endl;
  return 0;
}

int new_wait() {
  std::cerr << "wait clear func called" << std::endl;
  return 0;
}

int new_shutdown() {
  std::cerr << "shutdown called" << std::endl;
  return 0;
}

std::string readlink_string(const char* link_path) {
  char buffer[PATH_MAX];
  ssize_t len = readlink(link_path, buffer, sizeof(buffer) - 1);

  if (len == -1) {
    return "";
  }

  buffer[len] = '\0';
  return buffer;
}

__attribute__((visibility("default")))
int __libc_start_main(int (*_main)(int, char **, char **), int argc,
                      char **argv, int (*init)(int, char **, char **),
                      void (*fini)(void), void (*rtld_fini)(void),
                      void *stack_end) {

  if (ON_RM2) {
    auto binary_path = readlink_string("/proc/self/exe");

    if (binary_path.empty()) {
      std::cerr << "Unable to find current binary path\n";
      return -1;
    }

    if (binary_path == "/usr/bin/xochitl") {
      IN_XOCHITL = true;

      auto *update_fn = swtfb::locate_signature(
          binary_path.c_str(), "\x00\xd0\x4d\xe2\x00\x40\x8d\xe2\x00\x50\x8d\xe2", 12);
      if (update_fn == nullptr) {
        std::cerr << "Unable to find update fn" << std::endl;
        std::cerr << "PLEASE SEE "
                    "https://github.com/ddvk/remarkable2-framebuffer/issues/18"
                  << std::endl;
        return -1;
      }
      update_fn -= 8;

      auto *create_threads_fn = swtfb::locate_signature(
          binary_path.c_str(), "\x00\x40\xa0\xe1\x00\x52\x9f\xe5\x6b\x0d\xa0\xe3", 12);
      if (create_threads_fn == nullptr) {
        std::cerr << "Unable to find create threads fn" << std::endl;
        std::cerr << "PLEASE SEE "
                    "https://github.com/ddvk/remarkable2-framebuffer/issues/18"
                  << std::endl;
        return -1;
      }

      auto *wait_fn = swtfb::locate_signature(
          binary_path.c_str(), "\x01\x30\xa0\xe3\x30\x40\x9f\xe5", 8);
      if (wait_fn == nullptr) {
        std::cerr << "Unable to find wait threads fn" << std::endl;
        std::cerr << "PLEASE SEE "
                    "https://github.com/ddvk/remarkable2-framebuffer/issues/18"
                  << std::endl;
        return -1;
      }

      auto *shutdown_fn = swtfb::locate_signature(
          binary_path.c_str(), "\x01\x50\xa0\xe3\x44\x40\x9f\xe5", 8);
      if (shutdown_fn == nullptr) {
        std::cerr << "Unable to find shutdown fn" << std::endl;
        std::cerr << "PLEASE SEE "
                    "https://github.com/ddvk/remarkable2-framebuffer/issues/18"
                  << std::endl;
        return -1;
      }

      std::cerr << "Update fn address: " << std::hex << (void *)update_fn
                << "\nCreate th address: " << (void *)create_threads_fn
                << "\nWait for th address: " << (void *)wait_fn
                << "\nshutdown address: " << (void *)shutdown_fn << std::dec
                << std::endl;

      gum_init_embedded();
      GumInterceptor *interceptor = gum_interceptor_obtain();

      if (gum_interceptor_replace(interceptor, update_fn, (void *)new_update,
                                  nullptr) != GUM_REPLACE_OK) {
        std::cerr << "replace update fn error" << std::endl;
        return -1;
      }

      if (gum_interceptor_replace(interceptor, create_threads_fn,
                                  (void *)new_create_threads,
                                  nullptr) != GUM_REPLACE_OK) {
        std::cerr << "replace create threads fn error" << std::endl;
        return -1;
      }

      if (gum_interceptor_replace(interceptor, wait_fn, (void *)new_wait,
                                  nullptr) != GUM_REPLACE_OK) {
        std::cerr << "replace wait clear fn error" << std::endl;
        return -1;
      }

      if (gum_interceptor_replace(interceptor, shutdown_fn, (void *)new_shutdown,
                                  nullptr) != GUM_REPLACE_OK) {
        std::cerr << "replace shutdown fn error" << std::endl;
        return -1;
      }
    }
  }

  auto func_main =
      (typeof(&__libc_start_main))dlsym(RTLD_NEXT, "__libc_start_main");

  return func_main(_main, argc, argv, init, fini, rtld_fini, stack_end);
}
};
