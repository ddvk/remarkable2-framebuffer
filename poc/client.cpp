#include "ipc.cpp"

int msg_q_id = 0x2257c;
ipc::Queue MSGQ(msg_q_id);

int main(int argc, char *argv[]) {
  printf("SENDING MSG UPDATE\n");
  char* shared_mem = ipc::get_shared_buffer();
  ipc::msgbuf buf = { .mtype=0, .x = 1, .y = 2, .w = 3, .h = 4};
  MSGQ.send(buf);
}
