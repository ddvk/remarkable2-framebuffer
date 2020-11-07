#include <iostream>
#include <QGuiApplication>
#include <QImage>
#include <QObject>
#include <QPaintEngine>
#include <QPainter>
#include <QRect>
#include <QMetaObject>
#include <QMetaMethod>
#include <QDebug>

#include "now.cpp"
#include "qtdump.cpp"

using namespace std;

namespace swtfb {

// todo: make it singleton
class SwtFB {
  const int maxWidth = 1404;
  const int maxHeight = 1872;

public:
  SwtFB() {
    initQT();
  }

  void initQT() {
    qputenv("QMLSCENE_DEVICE", "epaper");
    qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");

    // needed for qpainter
    char *argv[0];
    int argc = 0;
    app = new QGuiApplication(argc, argv);
    auto ptr = f_getInstance();
    instance = reinterpret_cast<QObject*>(ptr);
    dump_qtClass(instance);
    img = (QImage *)(ptr + 8);
    // dump_qtClass(instance);



    cout << img->width() << " " << img->height() << " " << img->depth() << endl;
  }

  void clearScreen() {
    QObject *object = static_cast<QObject *>((QObject*) instance);
    QMetaObject::invokeMethod(instance,"clearScreen", Qt::DirectConnection);
  }

  void sendUpdate(QObject *a, QRect rect, int waveform, int flags=0, bool sync=0) {
    SendUpdate(rect, waveform, flags, sync);
  }

  void SendUpdate(const QRect &rect, int waveform, int flags, bool sync) {
      QGenericArgument argWaveform("EPFramebuffer::WaveformMode",&waveform);
      QGenericArgument argUpdateMode("EPFramebuffer::UpdateMode",&flags);
      QMetaObject::invokeMethod(instance,"sendUpdate", Qt::DirectConnection, Q_ARG(QRect, rect), argWaveform, argUpdateMode, Q_ARG(bool, sync));
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
    sendUpdate(instance, rect, 3, 3);
  }

  void DrawText(int i, char *text, bool wait=false) {
    QRect rect(0, i, 200, 100);
    QPainter painter(img);
    painter.drawText(rect, 132, text);
    painter.end();
    sendUpdate(instance, rect, 3, wait ? 0 : 1);
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
    sendUpdate(instance, rect, mode, 0, 0);
    // SendUpdate(rect, mode, 0, 0);

    #ifdef DEBUG
    cerr << get_now() << " Total Update took " << cz.elapsed() << "s" << endl;
    #endif
  }

private:
  QObject *instance;
  QGuiApplication *app;
  QImage *img;

  // black magic
  // fec600ccae7743dd4e5d8046427244c0
  uint32_t *(*f_getInstance)(void) = (uint32_t * (*)(void))0x21F54;
  void (*f_sendUpdate)(void *, ...) = (void (*)(void *, ...))0x21A34;
  //
  // dbd4e8dfeb8810c88fc9d5a902e65961
  // uint32_t *(*f_getInstance)(void) = (uint32_t * (*)(void))0x224BC;
  // void (*f_sendUpdate)(void *, ...) = (void (*)(void *, ...))0x2257C;
};
} // namespace swtfb
