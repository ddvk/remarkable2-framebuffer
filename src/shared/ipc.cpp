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

struct msg_rect {
  int x, y, w, h;

  msg_rect(int x = -1, int y = -1, int w = -1, int h = -1)
      : x(x), y(y), w(w), h(h) {
    (void)0;
  }
};

const int maxWidth = 1404;
const int maxHeight = 1872;
int BUF_SIZE = maxWidth * maxHeight; // hardcoded size of display mem for rM2
static char *get_shared_buffer(string name = "/swtfb.01") {
  if (name[0] != '/') {
    name = "/" + name;
  }

  int fd = shm_open(name.c_str(), O_RDWR | O_CREAT, S_IRWXU);
  if (fd == -1 && errno == 13) {
    fd = shm_open(name.c_str(), O_RDWR, S_IRWXU);
  }
  printf("SHM FD: %i, errno: %i\n", fd, errno);

  ftruncate(fd, BUF_SIZE);
  char* mem = (char*) mmap(NULL, BUF_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
  printf("OPENED SHARED MEM: /dev/shm%s\n", name.c_str());
  return mem;
}

class Queue {
public:
  unsigned long id;
  int msqid = -1;

  void init() { msqid = msgget(id, IPC_CREAT | 0600); }

  Queue(int id) : id(id) { init(); }

  void send(msg_rect msg) {
    auto r = msgsnd(msqid, (void *)&msg, sizeof(msg), 0);
    std::cout << "MSG Q SEND" << ' ' << msg.x << ' ' << msg.y << ' ' << msg.w
    << ' ' << msg.h << std::endl;
  }

  msg_rect recv() {
    msg_rect buf;
    auto len = msgrcv(msqid, &buf, sizeof(buf), 0, 0);
    if (len >= 0) {
      std::cout << "MSG Q RECV'D" << ' ' << buf.x << ' ' << buf.y << ' '
                << buf.w << ' ' << buf.h << std::endl;
      return buf;
    } else {
      std::cout << "ERR " << len << " " << errno << endl;
    }
    return msg_rect(-1, -1, -1, -1);
  }

  void destroy() { msgctl(msqid, IPC_RMID, 0); };
};
}; // namespace ipc
