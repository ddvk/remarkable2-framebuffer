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
#include "signature.cpp"

using namespace std;

namespace swtfb {

string SDK_BIN = "/usr/bin/remarkable-shutdown";
// todo: make it singleton
class SwtFB {
  const int maxWidth = 1404;
  const int maxHeight = 1872;

public:
  SwtFB() {
    setFunc();
    initQT();
  }

  void setFunc() {
    int* addr = (int*) locate_signature(SDK_BIN.c_str(), "|@\x9f\xe5|P\x9f\xe5", 8);
    if (addr != 0) {
      f_getInstance = (uint32_t * (*)(void))addr;
      fprintf(stderr, "ADDR: %x\n", addr);
    } else {
      fprintf(stderr, "COULDNT LOCATE SIGNATURE IN %s\n", SDK_BIN.c_str());
      fprintf(stderr, "PLEASE SEE https://github.com/ddvk/remarkable2-framebuffer/issues/18\n");
      exit(0);
    }
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
    img = (QImage *)(ptr + 8);
    dump_qtClass(instance);



    cout << img->width() << " " << img->height() << " " << img->depth() << endl;
  }

  void clearScreen() {
    QMetaObject::invokeMethod(instance,"clearScreen", Qt::DirectConnection);
  }

  void sendUpdate(QObject *a, QRect rect, int waveform, int flags=0, bool sync=0) {
    SendUpdate(rect, waveform, flags, sync);
  }

  void ClearGhosting() {
      QMetaObject::invokeMethod(instance,"setForceFull", Qt::DirectConnection, Q_ARG(bool,false));
  }

  void SendUpdate(const QRect &rect, int waveform, int flags, bool sync = false) {
      QGenericArgument argWaveform("EPFramebuffer::WaveformMode",&waveform);
      QGenericArgument argUpdateMode("EPFramebuffer::UpdateFlags",&flags);
      //QMetaObject::invokeMethod(instance,"sendUpdate", Qt::DirectConnection, Q_ARG(QRect, rect), argWaveform, argUpdateMode, Q_ARG(bool, sync));
      QMetaObject::invokeMethod(instance,"sendUpdate", Qt::DirectConnection, Q_ARG(QRect, rect), argWaveform, argUpdateMode);
      //QMetaObject::invokeMethod(instance,"sendUpdate", Qt::DirectConnection, Q_ARG(QRect, rect), argWaveform);
  }


  void DrawLine() {
    cout << "drawing a line " << endl;
    cout << "send update" << endl;
    for (int i = 1; i < maxWidth-4; i+=2) {
        for (int j = 1; j < maxHeight-2; j++) {
          QRect rect(i, j, 3, 3);
          img->setPixel(i+1, j+1, 0xFF);
          SendUpdate(rect, 1,4, 0);// 1,4 fast
        }
    }
  }
  void FullScreen(int color) {

    //ClearGhosting();
    //clearScreen();
    QRect rect(0, 0, maxWidth, maxHeight);
    img->fill(color);
    QPainter painter(img);
    painter.drawText(rect, 132, "Blah");
    painter.end();
    SendUpdate(rect, 2,3,0); //2,3 refresh

  }

  void DrawText(int i, char *text) {
    QRect rect(0, i, 200, 100);
    QPainter painter(img);
    painter.drawText(rect, 132, text);
    painter.end();
    SendUpdate(rect, 3, 0, 0);
  }

  void DrawRaw(uint16_t *buffer, int x, int y, int w, int h, int mode = 2, int async=0) {
    /* uint16_t *dest = (uint16_t *)img->bits(); */

    int stride = maxWidth;
    int x0 = x, y0 = y, x1 = x0 + w, y1 = y0 + h;

    /* for (int i = y0; i < y1; i++) { */
    /*   memcpy(&dest[i * stride + x0], &buffer[i * stride + x0], */
    /*          (x1 - x0) * sizeof(uint16_t)); */
    /* } */

    QRect rect(x, y, w, h);
    ClockWatch cz;
    sendUpdate(instance, rect, mode, 0, 0);

    #ifdef DEBUG
    cerr << get_now() << " Total Update took " << cz.elapsed() << "s" << endl;
    #endif
  }

private:
  QObject *instance;
  QGuiApplication *app;
  QImage *img;

  uint32_t *(*f_getInstance)(void) = (uint32_t * (*)(void))0;
  void (*f_sendUpdate)(void *, ...) = (void (*)(void *, ...))0;
};
} // namespace swtfb
