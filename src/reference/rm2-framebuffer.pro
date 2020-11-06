QT += gui

CONFIG += c++11 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        fbmanager.cpp \
        main.cpp \
        wbfreader.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /home/root/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    fbmanager.h \
    wbfreader.h
