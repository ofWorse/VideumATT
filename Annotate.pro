QT += core gui widgets

CONFIG += c++17

TEMPLATE = app
TARGET = Annotate

SOURCES += \
    src/annotationcanvas.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/projectmodel.cpp \
    src/trainmanager.cpp

HEADERS += \
    src/annotationcanvas.h \
    src/mainwindow.h \
    src/projectmodel.h \
    src/trainmanager.h

INCLUDEPATH += $$PWD/src

DEFINES += APP_SOURCE_DIR=\\\"$$PWD\\\"
