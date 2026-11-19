QT += core gui qml quick quickcontrols2 testlib dbus network
CONFIG += c++17 testcase
TARGET = boris_tests
TEMPLATE = app

INCLUDEPATH += ../src
LIBS += -lLayerShellQtInterface

HEADERS += \
    ../src/avatar.h \
    ../src/backend.h \
    ../src/config.h \
    ../src/cursor.h \
    ../src/filepicker.h \
    ../src/pet.h \
    ../src/portalfilepicker.h \
    ../src/theme.h \
    ../src/translator.h

SOURCES += \
    boris_tests.cpp \
    ../src/avatar.cpp \
    ../src/backend.cpp \
    ../src/config.cpp \
    ../src/cursor.cpp \
    ../src/pet.cpp \
    ../src/portalfilepicker.cpp \
    ../src/theme.cpp \
    ../src/translator.cpp
