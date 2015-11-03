QT      *= core gui widgets

CONFIG  *= c++11

SOURCES +=  \
    $$PWD/nativebrowser.cpp \
    $$PWD/nativebrowserimpl.cpp

win32:SOURCES += \
    $$PWD/nativebrowserimpl_win.cpp

macx:OBJECTIVE_SOURCES += \
    $$PWD/nativebrowserimpl_mac.mm

win32:LIBS *= -lOle32 -lOleAut32 -lGdi32
 macx:LIBS *= -framework WebKit -framework Foundation

HEADERS += \
    $$PWD/nativebrowser.h \
    $$PWD/nativebrowserimpl.h
