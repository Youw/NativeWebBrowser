QT      += core gui widgets

TEMPLATE = app
TARGET   = browser_test
DESTDIR  = D:/Projects/

SOURCES += main.cpp \
    form.cpp \
    nativebrowser.cpp \
    nativebrowserimpl.cpp \
    nativebrowserimpl_win.cpp

LIBS += -lOle32 -lOleAut32 -lGdi32

FORMS += \
    form.ui

HEADERS += \
    form.h \
    nativebrowser.h \
    nativebrowserimpl.h
