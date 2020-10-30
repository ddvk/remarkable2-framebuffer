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

using namespace std;

namespace ipc {
enum MSG_TYPE { INIT_t = 1, UPDATE_t };

// from mxcfb.h {{{
// NOTE: i put left in front of top instead of after
struct mxcfb_rect {
	uint32_t left;
	uint32_t top;
	uint32_t width;
	uint32_t height;
};

// TODO: decide if uint32_t is right size for temp, flags, dither_mode and quant_bit
// which were 'int' previously
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
int BUF_SIZE = maxWidth * maxHeight * sizeof(uint16_t); // hardcoded size of display mem for rM2
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
  uint16_t* mem = (uint16_t*) mmap(NULL, BUF_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
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
    cout << rect.left << " " << rect.top << " " << rect.width << " " << rect.height << endl;
    msgsnd(msqid, (void *)&msg, sizeof(msg), 0);
  }

  swtfb_update recv() {
    swtfb_update buf;
    auto len = msgrcv(msqid, &buf, sizeof(buf), 0, 0);
    if (len >= 0) {
      auto rect = buf.update_region;
      cout << rect.left << " " << rect.top << " " << rect.width << " " << rect.height << endl;
      return buf;
    } else {
      std::cout << "ERR " << len << " " << errno << endl;
    }

    return {};
  }

  void destroy() { msgctl(msqid, IPC_RMID, 0); };
};
}; // namespace ipc
