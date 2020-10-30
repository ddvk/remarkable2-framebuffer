#define __SH_BUILD
/* FILE: src/shared/ipc.cpp */

/* REQUIRES: */
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/limits.h>

namespace swtfb {
namespace ipc {

using namespace std;
enum MSG_TYPE { INIT_t = 1, UPDATE_t };

// from mxcfb.h {{{
// NOTE: i put left in front of top instead of after
struct mxcfb_rect {
  uint32_t left;
  uint32_t top;
  uint32_t width;
  uint32_t height;
};

// TODO: decide if uint32_t is right size for temp, flags, dither_mode and
// quant_bit which were 'int' previously
struct mxcfb_update_data {
  struct mxcfb_rect update_region;
  uint32_t waveform_mode;
  uint32_t update_mode;
  uint32_t update_marker;
  uint32_t temp;
  uint32_t flags;
  uint32_t dither_mode;
  uint32_t quant_bit;
  // struct mxcfb_alt_buffer_data alt_buffer_data;
};
// }}} end mxcfb.h

typedef struct mxcfb_rect swtfb_rect;
typedef struct mxcfb_update_data swtfb_update;

const int maxWidth = 1404;
const int maxHeight = 1872;
int BUF_SIZE = maxWidth * maxHeight *
               sizeof(uint16_t); // hardcoded size of display mem for rM2
static uint16_t *get_shared_buffer(string name = "/swtfb.01") {
  if (name[0] != '/') {
    name = "/" + name;
  }

  int fd = shm_open(name.c_str(), O_RDWR | O_CREAT, S_IRWXU);
  if (fd == -1 && errno == 13) {
    fd = shm_open(name.c_str(), O_RDWR, S_IRWXU);
  }
  printf("SHM FD: %i, errno: %i\n", fd, errno);

  ftruncate(fd, BUF_SIZE);
  uint16_t *mem =
      (uint16_t *)mmap(NULL, BUF_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
  printf("OPENED SHARED MEM: /dev/shm%s\n", name.c_str());
  return mem;
}

class Queue {
public:
  unsigned long id;
  int msqid = -1;

  void init() { msqid = msgget(id, IPC_CREAT | 0600); }

  Queue(int id) : id(id) { init(); }

  void send(swtfb_update msg) {
    // TODO: read return value
    auto rect = msg.update_region;
    cout << rect.left << " " << rect.top << " " << rect.width << " "
         << rect.height << endl;
    msgsnd(msqid, (void *)&msg, sizeof(msg), 0);
  }

  swtfb_update recv() {
    swtfb_update buf;
    auto len = msgrcv(msqid, &buf, sizeof(buf), 0, 0);
    if (len >= 0) {
      auto rect = buf.update_region;
      cout << rect.left << " " << rect.top << " " << rect.width << " "
           << rect.height << endl;
      return buf;
    } else {
      std::cout << "ERR " << len << " " << errno << endl;
    }

    return {};
  }

  void destroy() { msgctl(msqid, IPC_RMID, 0); };
};
}; // namespace ipc
}; // namespace swtfb


/* FILE: src/client/client.cpp */

/* REQUIRES:
src/shared/ipc.cpp */

#include <stdlib.h>
#include <unistd.h>

uint32_t WIDTH = 1404;
uint32_t HEIGHT = 1872;

namespace swtfb {

int msg_q_id = 0x2257c;
ipc::Queue MSGQ(msg_q_id);

class SwtFB {
public:
  uint16_t *fbmem;
  ipc::swtfb_update update;
  ipc::swtfb_rect dirty_area;

  SwtFB() {
    fbmem = ipc::get_shared_buffer();
    reset_dirty();
  }

  void reset_dirty() {
    dirty_area.left = WIDTH;
    dirty_area.top = HEIGHT;
    dirty_area.width = 0;
    dirty_area.height = 0;
  }

  void mark_dirty(ipc::swtfb_rect &&rect) { mark_dirty(rect); }

  void mark_dirty(ipc::swtfb_rect &rect) {
    uint32_t x1 = dirty_area.left + dirty_area.width;
    uint32_t y1 = dirty_area.top + dirty_area.height;

    x1 = std::max(x1, rect.left + rect.width);
    y1 = std::max(y1, rect.top + rect.height);

    if (x1 > WIDTH) {
      x1 = WIDTH - 1;
    }
    if (y1 > HEIGHT) {
      y1 = HEIGHT - 1;
    }

    dirty_area.left = std::min(rect.left, dirty_area.left);
    dirty_area.top = std::min(rect.top, dirty_area.top);

    dirty_area.width = x1 - dirty_area.left;
    dirty_area.height = y1 - dirty_area.top;
  }

  void redraw_screen(bool full_refresh = false) {
    ipc::swtfb_update update;
    if (full_refresh || dirty_area.width <= 0 || dirty_area.height <= 0) {
      ipc::swtfb_rect buf = {};
      buf.left = WIDTH;
      buf.top = HEIGHT;
      buf.width = 0;
      buf.height = 0;
      update.update_region = buf;
    } else {
      update.update_region = dirty_area;
    }

    MSGQ.send(update);
    reset_dirty();
  }
};
}

#ifndef __SH_BUILD
int main() {
  srand(time(NULL));
  printf("SENDING MSG UPDATE\n");

  swtfb::SwtFB fb;

  int offset = (rand() % 1024);

  for (unsigned int i = 0; i < WIDTH * HEIGHT; i++) {
    fb.fbmem[i] = i + offset;
  }

  uint32_t x = (rand() % WIDTH);
  uint32_t y = (rand() % HEIGHT);
  if (x > WIDTH) {
    x -= WIDTH;
  };
  if (y > HEIGHT) {
    y -= HEIGHT;
  };
  uint32_t w = 200 + (rand() % 10 + 1) * 50;
  uint32_t h = 200 + (rand() % 10 + 1) * 50;

  fb.mark_dirty({.left = x, .top = y, w, h});
  fb.redraw_screen();
}
#endif

