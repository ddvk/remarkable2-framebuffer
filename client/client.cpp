#include "../shared/ipc.cpp"

#include <stdlib.h>

ipc::Queue MSGQ;

int WIDTH = 1408;
int HEIGHT = 1040;

class SwtFB {
public:
  char *fbmem;
  ipc::msg_rect dirty_area;

  SwtFB() {
    fbmem = ipc::get_shared_buffer();
    reset_dirty();
  }

  void reset_dirty() {
    dirty_area.x = WIDTH;
    dirty_area.y = HEIGHT;
    dirty_area.w = -WIDTH;
    dirty_area.h = -HEIGHT;
  }

  void mark_dirty(ipc::msg_rect &&rect) { mark_dirty(rect); }

  void mark_dirty(ipc::msg_rect &rect) {
    int x1 = dirty_area.x + dirty_area.w;
    int y1 = dirty_area.y + dirty_area.h;

    x1 = max(x1, rect.x + rect.w);
    y1 = max(y1, rect.y + rect.h);

    dirty_area.x = min(rect.x, dirty_area.x);
    dirty_area.y = min(rect.y, dirty_area.y);

    dirty_area.w = x1 - dirty_area.x;
    dirty_area.h = y1 - dirty_area.y;
  }

  void redraw_screen(bool full_refresh = false) {
    if (full_refresh || dirty_area.w <= 0 || dirty_area.h <= 0) {
      ipc::msg_rect buf = {};
      buf.x = WIDTH;
      buf.y = HEIGHT;
      buf.w = 0;
      buf.h = 0;
      MSGQ.send(buf);
    } else {
      MSGQ.send(dirty_area);
    }
    reset_dirty();
  }
};

int main() {
  srand(time(NULL));
  printf("SENDING MSG UPDATE\n");

  SwtFB fb;

  for (int i = 0; i < 10; i++) {
    fb.fbmem[i] = rand();
    printf("%i, ", fb.fbmem[i]);
  }
  printf("\n");

  fb.mark_dirty({250, 500, 200, 200});
  fb.mark_dirty({250, 100, 200, 200});
  fb.redraw_screen();
}
