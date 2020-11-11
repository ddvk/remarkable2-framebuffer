//  g++ xofb.cpp -ldl -lrt -shared -fPIC -o xofb.so
//  LD_PRELOAD=xofb.so xochitl
//  /dev/shm/xofb now has the framebuffer of xochitl
//  this is 16bit rgb565

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

const int WIDTH = 1404;
const int HEIGHT = 1872;
const int SIZE = 2;

char *SHMEM = NULL;
bool FIRST_ALLOC = true;

extern "C" {
static void (*qImageCtor)(void *that, int x, int y, int f) = 0;
static void (*qImageCtorWithBuffer)(void *that, uint8_t *, int32_t x, int32_t y,
                                    int32_t bytes, int format, void (*)(void *),
                                    void *) = 0;
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() {
  int fd = shm_open("/xofb", O_RDWR | O_CREAT, 0755);
  fprintf(stderr, "SHM FD: %i, errno: %i\n", fd, errno);

  int BUF_SIZE = HEIGHT * WIDTH * SIZE;
  ftruncate(fd, BUF_SIZE);
  SHMEM = (char *)mmap(NULL, BUF_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);

  qImageCtor = (void (*)(void *, int, int, int))dlsym(
      RTLD_NEXT, "_ZN6QImageC1EiiNS_6FormatE");
  qImageCtorWithBuffer = (void (*)(
      void *, uint8_t *, int32_t, int32_t, int32_t, int, void (*)(void *),
      void *))dlsym(RTLD_NEXT, "_ZN6QImageC1EPhiiiNS_6FormatEPFvPvES2_");
}

void _ZN6QImageC1EiiNS_6FormatE(void *that, int x, int y, int f) {

  if (x == WIDTH && y == HEIGHT && FIRST_ALLOC) {
    fprintf(stderr, "REPLACING THE IMAGE with /dev/shm/xofb \n");

    FIRST_ALLOC = false;
    qImageCtorWithBuffer(that, (uint8_t *)SHMEM, WIDTH, HEIGHT, WIDTH * SIZE, f,
                         nullptr, nullptr);
    return;
  }
  qImageCtor(that, x, y, f);
}
};
