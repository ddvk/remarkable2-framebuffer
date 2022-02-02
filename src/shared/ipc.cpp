#include <iostream>
#include <string.h>
#include <string>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/limits.h>

#include "defines.h"
#include "now.cpp"

#include "mxcfb.h"

#ifndef O_RDWR
#define _FCNTL_H 1
#include <bits/fcntl.h>
#undef _FCNTL_H
#endif

namespace swtfb {
struct xochitl_data {
  int x1;
  int y1;
  int x2;
  int y2;

  int waveform;
  int flags;
};

struct wait_sem_data {
  char sem_name[512];
};

struct swtfb_update {
  long mtype;
  struct {
    union {
      xochitl_data xochitl_update;

      struct mxcfb_update_data update;
      wait_sem_data wait_update;

    };

#ifdef DEBUG_TIMING
    uint64_t ms;
#endif
  } mdata;
};

const int WIDTH = 1404;
const int HEIGHT = 1872;
inline void reset_dirty(mxcfb_rect &dirty_area) {
  dirty_area.left = WIDTH;
  dirty_area.top = HEIGHT;
  dirty_area.width = 0;
  dirty_area.height = 0;
}

inline void mark_dirty(mxcfb_rect &dirty_area, mxcfb_rect &rect) {
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

namespace ipc {

using namespace std;
enum MSG_TYPE { INIT_t = 1, UPDATE_t, XO_t, WAIT_t };

const int maxWidth = 1404;
const int maxHeight = 1872;
const int BUF_SIZE = maxWidth * maxHeight *
                     sizeof(uint16_t); // hardcoded size of display mem for rM2
int SWTFB_FD = 0;

// TODO: allow multiple shared buffers in one process?
static uint16_t *get_shared_buffer(string name = "/swtfb.01") {
  if (name[0] != '/') {
    name = "/" + name;
  }

  int fd = shm_open(name.c_str(), O_RDWR | O_CREAT, 0755);

  if (fd == -1 && errno == 13) {
    fd = shm_open(name.c_str(), O_RDWR | O_CREAT, 0755);
  }

  if (fd < 3) {
    fprintf(stderr, "SHM FD: %i, errno: %i\n", fd, errno);
  }
  SWTFB_FD = fd;

  ftruncate(fd, BUF_SIZE);
  uint16_t *mem =
      (uint16_t *)mmap(NULL, BUF_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);

  if (getenv("RM2FB_NESTED") == "") {
    fprintf(stderr, "OPENED SHARED MEM: /dev/shm%s at %x, errno: %i\n",
            name.c_str(), mem, errno);
  }
  return mem;
}

#define SWTFB1_UPDATE 1
class Queue {
public:
  unsigned long id;
  int msqid = -1;

  void init() { msqid = msgget(id, IPC_CREAT | 0600); }

  Queue(int id) : id(id) { init(); }

  void send(wait_sem_data data) {
    swtfb_update msg;
    msg.mtype = WAIT_t;
    msg.mdata.wait_update = data;

    int wrote = msgsnd(msqid, (void *)&msg, sizeof(msg.mdata.wait_update), 0);
    if (wrote != 0) {
      perror("Error sending wait update");
    }
  }

  void send(xochitl_data data) {
    swtfb_update msg;
    msg.mtype = XO_t;
    msg.mdata.xochitl_update = data;

    int wrote = msgsnd(msqid, (void *)&msg, sizeof(msg.mdata.xochitl_update), 0);
    if (wrote != 0) {
      perror("Error sending xochitl update");
    }
  }

  void send(mxcfb_update_data msg) {
// TODO: read return value
#ifdef DEBUG
    auto rect = msg.update_region;
    cerr << get_now() << " MSG Q SEND " << rect.left << " " << rect.top << " "
         << rect.width << " " << rect.height << endl;
#endif

    swtfb_update swtfb_msg;
    swtfb_msg.mtype = UPDATE_t;
    swtfb_msg.mdata.update = msg;

#ifdef DEBUG_TIMING
    swtfb_msg.ms = get_now();
#endif
    int wrote = msgsnd(msqid, (void *)&swtfb_msg, sizeof(swtfb_msg.mdata.update), 0);
    if (wrote != 0) {
      cerr << "ERRNO " << errno << endl;
    }
  }

  swtfb_update recv() {
    swtfb_update buf;
    errno = 0;
    auto len = msgrcv(msqid, &buf, sizeof(buf.mdata), 0, MSG_NOERROR);
#ifdef DEBUG_TIMING
    auto rect = buf.update.update_region;
    cerr << get_now() - buf.ms << "ms MSG Q RECV " << rect.left << " "
         << rect.top << " " << rect.width << " " << rect.height << endl;
#endif
    if (len >= 0) {
      return buf;
    } else {
      perror("Error recv msgbuf");
    }

    return {};
  }

  void destroy() { msgctl(msqid, IPC_RMID, 0); };
};
}; // namespace ipc
}; // namespace swtfb
