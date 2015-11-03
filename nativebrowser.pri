QT      *= core gui widgets

CONFIG  *= C++11

SOURCES +=  \
    nativebrowser.cpp \
    nativebrowserimpl.cpp

win32:SOURCES += \
    nativebrowserimpl_win.cpp

macx:OBJECTIVE_SOURCES += \
    nativebrowserimpl_mac.mm

win32:LIBS *= -lOle32 -lOleAut32 -lGdi32
 macx:LIBS *= -framework WebKit -framework Foundation

HEADERS += \
    nativebrowser.h \
    nativebrowserimpl.h
