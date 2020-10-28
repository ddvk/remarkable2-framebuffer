#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/limits.h>
#include <mqueue.h>

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

int BUF_SIZE = 0x165800; // hardcoded size of display mem for rM2
static char *get_shared_buffer(string name = "/swtfb.01") {
  if (name[0] != '/') {
    name = "/" + name;
  }

  int fd = shm_open(name.c_str(), O_RDWR, S_IRWXU);
  if (fd == -1 && errno == ENOENT) {
    printf("CREATING SHMEM\n");
    fd = shm_open(name.c_str(), O_RDWR, S_IRWXU);
    if (ftruncate(fd, BUF_SIZE) != 0) {
      printf("COULDNT RESIZE SHARED MEM\n");
    }
  }
  printf("SHM FD: %i, errno: %i\n", fd, errno);

  char *mem = (char *)mmap(NULL, BUF_SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
  printf("OPENED SHARED MEM: /dev/shm%s\n", name.c_str());
  return mem;
}

class Queue {
public:
  string name;
  mqd_t msqid = -1;

  void init() {
    mq_attr mqa;
    mqa.mq_maxmsg = 1;
    mqa.mq_msgsize = sizeof(msg_rect);
    msqid = mq_open(name.c_str(), O_CREAT | O_RDWR, S_IRWXU, &mqa);
  }

  Queue(string n = "/swtfb.01") {
    if (n[0] != '/') {
      n = "/" + n;
    }
    name = n;
    init();
  }

  void send(msg_rect msg) {
    mq_send(msqid, (char *)&msg, sizeof(msg), 0);
    std::cout << "MSG Q SEND" << ' ' << msg.x << ' ' << msg.y << ' ' << msg.w
              << ' ' << msg.h << std::endl;
  }

  msg_rect recv() {
    msg_rect buf;
    auto len = mq_receive(msqid, (char *)&buf, sizeof(buf), 0);
    if (len >= 0) {
      std::cout << "MSG Q RECV'D" << ' ' << buf.x << ' ' << buf.y << ' '
                << buf.w << ' ' << buf.h << std::endl;
      return buf;
    }
    return msg_rect(-1, -1, -1, -1);
  }

  void destroy() { mq_close(msqid); };
};
}; // namespace ipc
