#include "../shared/ipc.cpp"

#include <stdlib.h>

ipc::Queue MSGQ;


int main() {
  srand(time(NULL));
  printf("SENDING MSG UPDATE\n");
  char* shared_mem = ipc::get_shared_buffer();

  for (int i = 0; i < 10; i++) {
    shared_mem[i] = rand();
    printf("%i, ", shared_mem[i]);
  }
  printf("\n");

  ipc::msgbuf buf = { .mtype=0, .x = 1, .y = 2, .w = 3, .h = 4};
  MSGQ.send(buf);
}
