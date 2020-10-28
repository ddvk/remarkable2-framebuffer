#include "../shared/ipc.cpp"

ipc::Queue MSGQ;

int main() {
  printf("SENDING MSG UPDATE\n");
  char* shared_mem = ipc::get_shared_buffer();

  for (int i = 0; i < 10; i++) {
    shared_mem[0] = i;
  }

  ipc::msgbuf buf = { .mtype=0, .x = 1, .y = 2, .w = 3, .h = 4};
  MSGQ.send(buf);
}
