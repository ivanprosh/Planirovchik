QT += core
QT -= gui

TARGET = Lab1_3
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    Source/main.cpp

HEADERS += \
    Source/mythread.h

deployment.path = $$OUT_PWD/
deployment.files += input.ini \

INSTALLS += deployment

DISTFILES += \
    input.ini
