QT += core
QT -= gui

DESTDIR = ./

TARGET = SampleQt
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

win32: LIBS += -L$$PWD/../../exe/ -lmld

INCLUDEPATH += $$PWD/../../mld
DEPENDPATH += $$PWD/../../mld

win32:!win32-g++: PRE_TARGETDEPS += $$PWD/../../exe/mld.lib
else:win32-g++: PRE_TARGETDEPS += $$PWD/../../exe/libmld.a
