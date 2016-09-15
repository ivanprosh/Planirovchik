QT += core

CONFIG += c++11

TARGET = Lab1_14
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    Source/main.cpp \
    Source/stdafx.cpp

HEADERS += \
    Source/stdafx.h \
    Source/targetver.h \
    mythread.h \
    Source/mythread.h

deployment.path = $$OUT_PWD/
deployment.files += input.ini \

INSTALLS += deployment

DISTFILES += \
    input.ini
