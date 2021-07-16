#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QMetaMethod>
#include <QMetaObject>
#include <QObject>
#include <QPaintEngine>
#include <QPainter>
#include <QRect>
#include <iostream>

#include "now.cpp"
#include "qtdump.cpp"
#include "offsets.h"

using namespace std;

namespace swtfb {

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
    const auto offsets = read_offsets();
    auto search = offsets.find("getInstance");

    if (search == offsets.end()) {
      std::cerr << "No offset defined for function 'getInstance'\n"
        "PLEASE SEE https://github.com/ddvk/remarkable2-framebuffer/issues/18\n";
      std::exit(-1);
    }

    f_getInstance = (uint32_t * (*)(void)) search->second;
    std::cerr << "getInstance() at address: " << search->second << '\n';
  }

  void initQT() {
    auto ptr = f_getInstance();
    instance = reinterpret_cast<QObject *>(ptr);
    img = (QImage *)(ptr + 8);

#ifdef DEBUG
    dump_qtClass(instance);
#endif

    cout << img->width() << " " << img->height() << " " << img->depth() << endl;
  }

  void ClearScreen() {
    QMetaObject::invokeMethod(instance, "clearScreen", Qt::DirectConnection);
  }

  void ClearGhosting() {
    QMetaObject::invokeMethod(instance, "setForceFull", Qt::DirectConnection,
                              Q_ARG(bool, false));
  }

  void SendUpdate(const QRect &rect, int waveform, int flags) const {
    // Method idx == 1
    QGenericArgument argWaveform("EPFramebuffer::WaveformMode", &waveform);
    QGenericArgument argUpdateMode("EPFramebuffer::UpdateFlags", &flags);
    QMetaObject::invokeMethod(instance, "sendUpdate", Qt::DirectConnection,
                              Q_ARG(QRect, rect), argWaveform, argUpdateMode);
  }
  void WaitForLastUpdate() const {
    // TODO: method Idx == 5, just returns.
    QMetaObject::invokeMethod(instance, "waitForLastUpdate",
                              Qt::DirectConnection);
  }

  void DrawLine() {
    cout << "drawing a line " << endl;
    cout << "send update" << endl;
    for (int i = 1; i < maxWidth - 4; i += 2) {
      for (int j = 1; j < maxHeight - 2; j++) {
        QRect rect(i, j, 3, 3);
        img->setPixel(i + 1, j + 1, 0xFF);
        SendUpdate(rect, 1, 4); // 1,4 fast
      }
    }
  }
  void FullScreen(int color) {

    // ClearGhosting();
    // clearScreen();
    QRect rect(0, 0, maxWidth, maxHeight);
    img->fill(color);
    QPainter painter(img);
    painter.drawText(rect, 132, "Blah");
    painter.end();
    SendUpdate(rect, 2, 3); // 2,3 refresh
  }

  void DrawText(int i, char *text) {
    QRect rect(0, i, 200, 100);
    QPainter painter(img);
    painter.drawText(rect, 132, text);
    painter.end();
    SendUpdate(rect, 3, 0);
  }

  void DrawRaw(int x, int y, int w, int h, int waveform = 2,
               int flags = 3) const {
    QRect rect(x, y, w, h);
    ClockWatch cz;
    SendUpdate(rect, waveform, flags);

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
