#include <iostream>
#include <string.h>
#include <string>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/limits.h>

#include "mxcfb.h"

#ifndef O_RDWR
/* File access modes for `open' and `fcntl'.  */
#define        O_RDONLY        0        /* Open read-only.  */
#define        O_WRONLY        1        /* Open write-only.  */
#define        O_RDWR                2        /* Open read/write.  */
/* Bits OR'd into the second argument to open.  */
#define        O_CREAT                0x0200        /* Create file if it doesn't exist.  */
#endif

namespace swtfb {
struct swtfb_update {
  long mtype;
  struct mxcfb_update_data update;
};

int WIDTH = 1404;
int HEIGHT = 1872;
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
enum MSG_TYPE { INIT_t = 1, UPDATE_t };


const int maxWidth = 1404;
const int maxHeight = 1872;
int BUF_SIZE = maxWidth * maxHeight *
               sizeof(uint16_t); // hardcoded size of display mem for rM2
int SWTFB_FD = 0;

// TODO: allow multiple shared buffers in one process?
static uint16_t *get_shared_buffer(string name = "/swtfb.01") {
  if (name[0] != '/') {
    name = "/" + name;
  }

  int fd = shm_open(name.c_str(), O_RDWR | O_CREAT, 0755);

  if (fd == -1 && errno == 13) {
    fd = shm_open(name.c_str(), O_RDWR, 0755);
  }

  fprintf(stderr, "SHM FD: %i, errno: %i\n", fd, errno);
  SWTFB_FD = fd;

  ftruncate(fd, BUF_SIZE);
  uint16_t *mem =
      (uint16_t *)mmap(NULL, BUF_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
  fprintf(stderr, "OPENED SHARED MEM: /dev/shm%s at %x, errno: %i\n", name.c_str(), mem, errno);
  return mem;
}

#define SWTFB1_UPDATE 1
class Queue {
public:
  unsigned long id;
  int msqid = -1;

  void init() { msqid = msgget(id, IPC_CREAT | 0600); }

  Queue(int id) : id(id) { init(); }

  void send(mxcfb_update_data msg) {
    // TODO: read return value
    #ifdef DEBUG
    auto rect = msg.update_region;
    cerr << " MSG Q SEND " << rect.left << " " << rect.top << " "
         << rect.width << " " << rect.height << endl;
    #endif

    swtfb_update swtfb_msg;
    swtfb_msg.mtype = 1;
    swtfb_msg.update = msg;
    int wrote = msgsnd(msqid, (void *)&swtfb_msg, sizeof(swtfb_msg), 0);
    if (wrote != 0) {
      cerr << "ERRNO " << errno << endl;
    }
  }

  mxcfb_update_data recv() {
    swtfb_update buf;
    auto len = msgrcv(msqid, &buf, sizeof(buf), 0, 0);
    #ifdef DEBUG
    auto rect = buf.update.update_region;
    cerr << " MSG Q RECV " << rect.left << " " << rect.top << " "
         << rect.width << " " << rect.height << endl;
    #endif
    if (len >= 0) {
      return buf.update;
    } else {
      cerr << "ERR " << len << " " << errno << endl;
    }

    return {};
  }

  void destroy() { msgctl(msqid, IPC_RMID, 0); };
};
}; // namespace ipc
}; // namespace swtfb
