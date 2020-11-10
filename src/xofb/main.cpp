//  g++ xofb.cpp -ldl -lrt -shared -fPIC -o xofb.so

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

unsigned int BUF_SIZE = 1404 * 1872 * 2;
char *SHMEM = NULL;
bool FIRST_ALLOC = true;

extern "C" {
static void _libhook_init() __attribute__((constructor));
static void _libhook_init() {
  int fd = shm_open("/xofb", O_RDWR | O_CREAT, 0755);
  fprintf(stderr, "SHM FD: %i, errno: %i\n", fd, errno);

  ftruncate(fd, BUF_SIZE);
  SHMEM = (char *)mmap(NULL, BUF_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
}

bool malloced = false;
void *malloc(unsigned int size) {
  static void *(*func_malloc)(unsigned int) = 0;

  if (size == BUF_SIZE && FIRST_ALLOC) {
    fprintf(stderr, "MALLOC %i INTO /dev/shm/xofb\n", size);
    FIRST_ALLOC = false;
    return SHMEM;
  }

  if (!func_malloc) {
    func_malloc = (void *(*)(unsigned int))dlsym(RTLD_NEXT, "malloc");
  }

  auto r = func_malloc(size);
  return r;
}
};
