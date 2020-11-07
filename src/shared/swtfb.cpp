#include <QGuiApplication>
#include <QImage>
#include <QPaintEngine>
#include <QPainter>
#include <QRect>
#include <iostream>

#include <chrono>
#include "now.cpp"

using namespace std;

namespace swtfb {
class ClockWatch {
public:
  chrono::high_resolution_clock::time_point t1;

  ClockWatch() { t1 = chrono::high_resolution_clock::now(); }

  auto elapsed() {
    auto t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> time_span =
        chrono::duration_cast<chrono::duration<double>>(t2 - t1);
    return time_span.count();
  }
};

// todo: make it singleton
class SwtFB {
  const int maxWidth = 1404;
  const int maxHeight = 1872;

public:
  SwtFB() { initQT(); }

  void initQT() {
    qputenv("QMLSCENE_DEVICE", "epaper");
    qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");

    // needed for qpainter
    char *argv[0];
    int argc = 0;
    app = new QGuiApplication(argc, argv);
    instance = getInstance();
    img = (QImage *)(instance + 8);
    printf("INSTANCE ADDRESS: 0x%lx\n", instance);

    cout << img->width() << " " << img->height() << " " << img->depth() << endl;
  }

  void DrawLine() {
    cout << "drawing a line " << endl;
    cout << "send update" << endl;
    for (int i = 100; i < maxHeight; i++) {
      QRect rect(100, i, 200, 100);
      img->setPixel(100, i, 0xFF);
      printf(".");
      sendUpdate(instance, rect, 1, 0);
    }
  }
  void FullScreen() {
    QRect rect(0, 0, maxWidth, maxHeight);
    QPainter painter(img);
    painter.drawText(rect, 132, "Blah");
    painter.end();
    // sendUpdate(0, rect, 3, 3); //slow
    // sendUpdate(0, rect, 3, 2); //slow but nice
    // sendUpdate(0, rect, 2, 1); //flashing background
    // sendUpdate(0, rect, 2, 0); //flashing background
    sendUpdate(instance, rect, 3, 3);
  }

  void DrawText(int i, char *text, bool wait) {
    QRect rect(0, i, 200, 100);
    QPainter painter(img);
    painter.drawText(rect, 132, text);
    painter.end();
    int w = wait ? 2 : 0;
    // sendUpdate(0, rect, 3, 3); //slow
    // sendUpdate(0, rect, 3, 2); //slow but nice
    // sendUpdate(0, rect, 2, 1); //flashing background
    // sendUpdate(0, rect, 2, 0); //flashing background
    sendUpdate(instance, rect, 3, 2);
  }

  void DrawRaw(uint16_t *buffer, int x, int y, int w, int h, int mode = 2, int async=0) {
    uint16_t *dest = (uint16_t *)img->bits();

    int stride = maxWidth;
    int x0 = x, y0 = y, x1 = x0 + w, y1 = y0 + h;

    for (int i = y0; i < y1; i++) {
      memcpy(&dest[i * stride + x0], &buffer[i * stride + x0],
             (x1 - x0) * sizeof(uint16_t));
    }

    QRect rect(x, y, w, h);
    ClockWatch cz;
    // 2nd param:
    // 5 is flashing
    sendUpdate(instance, rect, mode, async);
    cerr << get_now() << " Total Update took " << cz.elapsed() << "s" << endl;
  }

private:
  uint32_t *instance;
  QGuiApplication *app;
  QImage *img;

  // black magic
  // fec600ccae7743dd4e5d8046427244c0
  uint32_t *(*getInstance)(void) = (uint32_t * (*)(void))0x21F54;
  void (*sendUpdate)(void *, ...) = (void (*)(void *, ...))0x21A34;
  //
  // dbd4e8dfeb8810c88fc9d5a902e65961
  // uint32_t *(*getInstance)(void) = (uint32_t * (*)(void))0x224BC;
  // void (*sendUpdate)(void *, ...) = (void (*)(void *, ...))0x2257C;
};
} // namespace swtfb
