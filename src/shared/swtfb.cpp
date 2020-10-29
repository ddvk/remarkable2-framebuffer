#include <QImage>
#include <QGuiApplication>
#include <QPaintEngine>
#include <QPainter>
#include <QRect>
#include <iostream>

#include <chrono>

using namespace std;

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
  SwtFB() {
    instance = NULL;
    initQT();
  }

  void initQT() {
    qputenv("QMLSCENE_DEVICE", "epaper");
    qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");

    // needed for qpainter
    char *argv[0];
    int argc = 0;
    app = new QGuiApplication(argc, argv);
    instance = getInstance();
    img = (QImage *)(instance + 8);
    cout << "INSTANCE ADDRESS: " << std::hex << instance << std::dec << std::endl;

    std::cout << img->width() << " " << img->height() << " " << img->depth() << std::endl;
  }

  void DrawLine() {
    if (!instance) { return; }
      for(int i=0; i < maxHeight; i++) {
          img->setPixel(100, i, 0xF0);
          QRect rect(99, i+1, 2, 2);
          sendUpdate(0, rect, 1, 4); 
          printf(".");
      }
  }

  void DrawText(int i, char *text, bool wait) {
    if (!instance) { return; }
    QRect rect(100, i, 200, 100);
    QPainter painter(img);
    painter.drawText(rect, 132, text);
    painter.end();
    int w = wait ? 2 : 0;
    //sendUpdate(0, rect, 3, 3); //slow
    //sendUpdate(0, rect, 3, 2); //slow but nice
    //sendUpdate(0, rect, 2, 1); //flashing background
    //sendUpdate(0, rect, 2, 0); //flashing background
    sendUpdate(0, rect, 2, 4); 
  }

  void DrawRaw(uint16_t *buffer, int x, int y, int w, int h) {
    if (!instance) { return; }
    uint16_t* dest = (uint16_t*) img->bits();

    int stride = maxWidth;
    int x0 = x, y0 = y, x1 = x0 + w, y1 = y0 + h;

    std::cout << "CORNERS" << " " << x0 << " " << y0 << " " << x1 << " " << y1 << std::endl;

    for (int i = y0; i < y1; i++) {
      memcpy(&dest[i * stride + x0], &buffer[i * stride + x0],
             (x1 - x0) * sizeof(uint16_t));
    }

    QRect rect(x, y, w, h);
    ClockWatch cz;
    sendUpdate(0, rect, 3, 2);
    std::cout << "Update took " << cz.elapsed() << "s" << std::endl;
  }

private:
  uint32_t *instance;
  QGuiApplication *app;
  QImage *img;

  // black magic
  uint32_t *(*getInstance)(void) = (uint32_t * (*)(void))0x224BC;
  void (*sendUpdate)(void *, ...) = (void (*)(void *, ...))0x2257C;
};
