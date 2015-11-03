QT      += core gui widgets

TEMPLATE = app
TARGET   = browser_test
DESTDIR  = $$PWD/bin
CONFIG  += C++11

SOURCES += main.cpp \
    form.cpp \
    nativebrowser.cpp \
    nativebrowserimpl.cpp

win32:SOURCES += \
    nativebrowserimpl_win.cpp

macx:OBJECTIVE_SOURCES += \
    nativebrowserimpl_mac.mm

win32:LIBS += -lOle32 -lOleAut32 -lGdi32
 macx:LIBS += -framework WebKit -framework Foundation

FORMS += \
    form.ui

HEADERS += \
    form.h \
    nativebrowser.h \
    nativebrowserimpl.h

QMAKE_INFO_PLIST = Info.plist
