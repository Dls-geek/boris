QT += core gui qml quick quickcontrols2 dbus network

CONFIG += c++17 release
TARGET = boris
TEMPLATE = app

# Layer shell is how a window this small sits on the desktop without taking it over.
LIBS += -lLayerShellQtInterface

HEADERS += \
    src/avatar.h \
    src/backend.h \
    src/config.h \
    src/cursor.h \
    src/filepicker.h \
    src/pet.h \
    src/portalfilepicker.h \
    src/theme.h \
    src/translator.h

SOURCES += \
    src/avatar.cpp \
    src/backend.cpp \
    src/config.cpp \
    src/cursor.cpp \
    src/main.cpp \
    src/pet.cpp \
    src/portalfilepicker.cpp \
    src/theme.cpp \
    src/translator.cpp

RESOURCES += src/resources.qrc
