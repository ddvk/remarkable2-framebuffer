#include <dlfcn.h>
#include <iostream>
#include <libgen.h>
#include <linux/fb.h>
#include <linux/ioctl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

#include <QByteArray>

#include "../shared/ipc.cpp"
#include "../shared/signature.cpp"

#include "frida/frida-gum.h"

#define FB_ID "mxcfb"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

int msg_q_id = 0x2257c;
swtfb::ipc::Queue MSGQ(msg_q_id);

uint16_t *SHARED_BUF;

const int SIZE = 2;

bool IN_XOCHITL = false;

extern "C" {
struct stat;

// QImage(width, height, format)
static void (*qImageCtor)(void *that, int x, int y, int f) = 0;
// QImage(uchar*, width, height, bytesperline, format)
static void (*qImageCtorWithBuffer)(void *that, uint8_t *, int32_t x, int32_t y,
                                    int32_t bytes, int format, void (*)(void *),
                                    void *) = 0;

static void _libhook_init() __attribute__((constructor));
static void _libhook_init() {
  std::ios_base::Init i;

  auto VERSION = "0.1";

  setenv("RM2FB_SHIM", VERSION, true);

  if (getenv("RM2FB_ACTIVE") != "") {
    setenv("RM2FB_NESTED", "1", true);
  } else {
    setenv("RM2FB_ACTIVE", "1", true);
  }

  qImageCtor = (void (*)(void *, int, int, int))dlsym(
      RTLD_NEXT, "_ZN6QImageC1EiiNS_6FormatE");
  qImageCtorWithBuffer = (void (*)(
      void *, uint8_t *, int32_t, int32_t, int32_t, int, void (*)(void *),
      void *))dlsym(RTLD_NEXT, "_ZN6QImageC1EPhiiiNS_6FormatEPFvPvES2_");

  SHARED_BUF = swtfb::ipc::get_shared_buffer();
}

bool FIRST_ALLOC = true;
void _ZN6QImageC1EiiNS_6FormatE(void *that, int x, int y, int f) {
  if (IN_XOCHITL && x == swtfb::WIDTH && y == swtfb::HEIGHT && FIRST_ALLOC) {
    fprintf(stderr, "REPLACING THE IMAGE with /dev/shm/xofb \n");

    FIRST_ALLOC = false;
    qImageCtorWithBuffer(that, (uint8_t *)SHARED_BUF, swtfb::WIDTH,
                         swtfb::HEIGHT, swtfb::WIDTH * SIZE, f, nullptr,
                         nullptr);
    return;
  }
  qImageCtor(that, x, y, f);
}

int open64(const char *pathname, int flags, mode_t mode = 0) {
  static int (*func_open)(const char *, int, mode_t) = NULL;

  if (!func_open) {
    func_open = (int (*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open64");
  }

  auto r = func_open(pathname, flags, mode);
  if (not IN_XOCHITL) {
    if (pathname == std::string("/dev/fb0")) {
      return swtfb::ipc::SWTFB_FD;
    }
  }

  return r;
}

int open(const char *pathname, int flags, mode_t mode = 0) {
  static int (*func_open)(const char *, int, mode_t) = NULL;

  if (!func_open) {
    func_open = (int (*)(const char *, int, mode_t))dlsym(RTLD_NEXT, "open");
  }

  auto r = func_open(pathname, flags, mode);
  if (not IN_XOCHITL) {
    if (pathname == std::string("/dev/fb0")) {
      return swtfb::ipc::SWTFB_FD;
    }
  }

  return r;
}

int ioctl(int fd, unsigned long request, char *ptr) {
  static int (*func_ioctl)(int, unsigned long, ...) = NULL;
  if (not IN_XOCHITL) {

    if (fd == swtfb::ipc::SWTFB_FD) {
      if (request == MXCFB_SEND_UPDATE) {

        mxcfb_update_data *update = (mxcfb_update_data *)ptr;
        MSGQ.send(*update);
        return 0;
      } else if (request == MXCFB_SET_AUTO_UPDATE_MODE) {

        return 0;
      } else if (request == MXCFB_WAIT_FOR_UPDATE_COMPLETE) {
        std::cerr << "CLIENT: sync" << std::endl;
        return 0;
      }

      else if (request == FBIOGET_VSCREENINFO) {

        fb_var_screeninfo *screeninfo = (fb_var_screeninfo *)ptr;
        screeninfo->xres = 1404;
        screeninfo->yres = 1872;
        screeninfo->grayscale = 0;
        screeninfo->bits_per_pixel = 16;
        return 0;
      }

      else if (request == FBIOPUT_VSCREENINFO) {

        return 0;
      } else if (request == FBIOGET_FSCREENINFO) {

        fb_fix_screeninfo *screeninfo = (fb_fix_screeninfo *)ptr;
        screeninfo->smem_len = swtfb::ipc::BUF_SIZE;
        screeninfo->smem_start = (unsigned long)SHARED_BUF;
        screeninfo->line_length = swtfb::WIDTH * sizeof(uint16_t);
        memcpy(screeninfo->id, FB_ID, sizeof(FB_ID));
        screeninfo->id[sizeof(FB_ID)] = 0;
        return 0;
      } else {
        std::cerr << "UNHANDLED IOCTL" << ' ' << request << std::endl;
        return 0;
      }
    }
  }

  if (!func_ioctl) {
    func_ioctl =
        (int (*)(int, unsigned long request, ...))dlsym(RTLD_NEXT, "ioctl");
  }

  return func_ioctl(fd, request, ptr);
}

static const auto touchArgs = QByteArray("rotate=180:invertx");

bool _Z7qputenvPKcRK10QByteArray(const char *name, const QByteArray &val) {
  auto orig_fn = (bool (*)(const char *, const QByteArray &))dlsym(
      RTLD_NEXT, "_Z7qputenvPKcRK10QByteArray");

  if (strcmp(name, "QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS") == 0) {
    return orig_fn(name, touchArgs);
  }

  return orig_fn(name, val);
}

void (*old_update)(void *, int, int, int, int, int, int) = 0;
int (*old_create_threads)(const char *, void *) = 0;
int (*old_wait)(void) = 0;
int (*old_shutdown)(void) = 0;

void new_update(void *instance, int x1, int y1, int x2, int y2, int waveform,
                int flags) {
  std::cerr << "UPDATE HOOK CALLED" << std::endl;
  std::cerr << "x " << x1 << " " << x2 << std::endl;
  std::cerr << "y " << y1 << " " << y2 << std::endl;
  std::cerr << "wav " << waveform << " flags " << flags << std::endl;

  swtfb::xochitl_data data;
  data.x1 = x1;
  data.x2 = x2;
  data.y1 = y1;
  data.y2 = y2;
  data.waveform = waveform;
  data.flags = flags;
  MSGQ.send(data);

  return;
  // return old_update(instance, x1, y1, x2, y2, waveform, flags);
}

int new_create_threads(const char *path, void *buf) {
  std::cerr << "create threads called" << std::endl;
  return 0;
  // return old_create_threads(path, buf);
}

int new_wait(void) {
  std::cerr << "wait func called" << std::endl;
  return 0;
  // return old_wait();
}

int new_shutdown(void) {
  std::cerr << "shutdown called" << std::endl;
  return 0;
}

GumInterceptor *interceptor;

int __libc_start_main(int (*_main)(int, char **, char **), int argc,
                      char **argv, int (*init)(int, char **, char **),
                      void (*fini)(void), void (*rtld_fini)(void),
                      void *stack_end) {

  if (std::string(argv[0]).find("xochitl") != std::string::npos) {
    IN_XOCHITL = true;

    auto *update_fn = swtfb::locate_signature(
                          argv[0], "\x10\xd0\x4d\xe2\x7b\xc5\x00\xe3", 8) +
                      4;
    auto *create_threads_fn =
        swtfb::locate_signature(
            argv[0], "\xc0\x64\x11\x00\x70\x40\x2d\xe9\x00\x40\xa0\xe1", 12) +
        8;

    auto *wait_fn = swtfb::locate_signature(
                        argv[0], "\x40\x6c\x11\x00\x10\x40\x2d\xe9", 8) +
                    8;

    auto *shutdown_fn = swtfb::locate_signature(
                            argv[0], "\x70\x57\x16\x00\x70\x40\x2d\xe9", 8) +
                        8;

    std::cerr << "Update fn address: " << std::hex << (void *)update_fn
              << "\nCreate th address: " << (void *)create_threads_fn
              << "\nWait for th address: " << (void *)wait_fn
              << "\nshutdown address: " << (void *)shutdown_fn << std::dec
              << std::endl;

    old_update = (decltype(old_update))update_fn;
    old_create_threads = (decltype(old_create_threads))create_threads_fn;
    old_wait = (decltype(old_wait))wait_fn;
    old_shutdown = (decltype(old_shutdown))shutdown_fn;

    gum_init_embedded();
    interceptor = gum_interceptor_obtain();

    if (gum_interceptor_replace(interceptor, update_fn, (void *)new_update,
                                nullptr) != GUM_REPLACE_OK) {
      std::cerr << "replace error" << std::endl;
    }

    if (gum_interceptor_replace(interceptor, create_threads_fn,
                                (void *)new_create_threads,
                                nullptr) != GUM_REPLACE_OK) {
      std::cerr << "replace error" << std::endl;
    }

    if (gum_interceptor_replace(interceptor, wait_fn, (void *)new_wait,
                                nullptr) != GUM_REPLACE_OK) {
      std::cerr << "replace error" << std::endl;
    }

    if (gum_interceptor_replace(interceptor, shutdown_fn, (void *)new_shutdown,
                                nullptr) != GUM_REPLACE_OK) {
      std::cerr << "replace error" << std::endl;
    }
    // exit(0);
  }

  typeof(&__libc_start_main) func_main =
      (typeof(&__libc_start_main))dlsym(RTLD_NEXT, "__libc_start_main");

  return func_main(_main, argc, argv, init, fini, rtld_fini, stack_end);
}
};

int main() { (void)0; };
