#define len(x) (int)(x).size()

#include <iostream>
#include <string.h>
#include <string>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include <linux/limits.h>

using namespace std;

namespace ipc {
enum MSG_TYPE { INIT_t = 1, UPDATE_t };

struct msgbuf {
  int mtype;

  int x, y, w, h;

  msgbuf(int mtype = UPDATE_t, int x = -1, int y = -1, int w = -1, int h = -1)
      : mtype(mtype), x(x), y(y), w(w), h(h) {
    (void)0;
  }
};

class Queue {
public:
  unsigned long id;
  int msqid = -1;

  void init() { msqid = msgget(id, IPC_CREAT | 0600); }

  Queue(int id) : id(id) { init(); }

  void send(msgbuf msg) {
    auto r = msgsnd(msqid, (void *)&msg, sizeof(msg), 0);
    std::cout << "MSG Q SEND" << ' ' << msg.mtype << ' ' << msg.x << ' '
              << msg.y << ' ' << msg.w << ' ' << msg.h << std::endl;
  }

  msgbuf recv() {
    msgbuf buf;
    auto len = msgrcv(msqid, &buf, sizeof(buf), 0, IPC_NOWAIT);
    if (len >= 0) {
      std::cout << "MSG Q RECV'D" << ' ' << buf.mtype << ' ' << buf.x << ' '
                << buf.y << ' ' << buf.w << ' ' << buf.h << std::endl;
      return buf;
    }
    return msgbuf(-1, -1, -1, -1);
  }

  void destroy() { msgctl(msqid, IPC_RMID, 0); };
};
}; // namespace ipc
