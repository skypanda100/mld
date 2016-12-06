QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DESTDIR = ./

TARGET = SampleQt

TEMPLATE = app

SOURCES += main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

win32: LIBS += -L$$PWD/../../exe/ -lmld

INCLUDEPATH += $$PWD/../../mld
DEPENDPATH += $$PWD/../../mld

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../../exe/mld.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../../exe/libmld.a


