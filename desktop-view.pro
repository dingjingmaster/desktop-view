QT       += core gui gui-private

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets widgets-private

CONFIG += c++11

SOURCES += \
    filesystem-model.cpp \
    src/desktop-view.cpp \
    src/example.cpp \
    src/screen.cpp

HEADERS += \
    filesystem-model.h \
    src/desktop-view.h \
    src/screen.h
