#include "client.cpp"

uint16_t BLACK = 0x0;
uint16_t WHITE = 0xFFFF; // 16 bits

int main(int argc, char *argv[]) {
  srand(time(NULL));
  printf("SENDING MSG UPDATE\n");

  swtfb::SwtFB fb;

  for (unsigned int i = 0; i < WIDTH * HEIGHT; i++) {
    fb.fbmem[i] = i / WIDTH;
  }

  for (unsigned int i = 0; i < WIDTH; i++) {
    fb.fbmem[i*WIDTH + 200] = BLACK;
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
