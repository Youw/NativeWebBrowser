QT      *= core gui widgets

TEMPLATE = app
TARGET   = browser_test
DESTDIR  = $$PWD/bin
CONFIG  += C++11

include(nativebrowser.pri)

SOURCES += main.cpp \
    form.cpp

FORMS += \
    form.ui

HEADERS += \
    form.h

QMAKE_INFO_PLIST = Info.plist
