#-------------------------------------------------
#
# Project created by QtCreator 2014-12-27T20:50:36
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = digilock
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    rs232.c \
    req.c \
    FPS_GT511Linux.cpp \
    db.c \
    bitmap.c \
    scanner.cpp \
    dictionary.c \
    iniparser.c \
    intercom.cpp

HEADERS += \
    rs232.h \
    req.h \
    ports.h \
    FPS_GT511Linux.h \
    db.h \
    bitmap.h \
    scanner.h \
    base64.h \
    dictionary.h \
    iniparser.h \
    intercom.h


unix: CONFIG += link_pkgconfig
unix: PKGCONFIG += libcurl
unix: PKGCONFIG += sqlite3

OTHER_FILES += \
    config.ini
