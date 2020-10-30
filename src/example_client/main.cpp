#include "../../dist/librm2fb.hpp"

int main() {
  srand(time(NULL));
  printf("SENDING MSG UPDATE\n");

  swtfb::SwtFB fb;

  int offset = (rand() % 1024);

  for (unsigned int i = 0; i < WIDTH * HEIGHT; i++) {
    fb.fbmem[i] = i + offset;
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
