QT += core gui qml quick quickcontrols2 dbus network multimedia

CONFIG += c++17 release
TARGET = boris
TEMPLATE = app

# Layer shell is how a window this small sits on the desktop without taking it over.
LIBS += -lLayerShellQtInterface

HEADERS += \
    src/avatar.h \
    src/backend.h \
    src/chirps.h \
    src/config.h \
    src/cursor.h \
    src/filepicker.h \
    src/personality.h \
    src/pet.h \
    src/portalfilepicker.h \
    src/theme.h \
    src/translator.h \
    src/wakeword.h

SOURCES += \
    src/avatar.cpp \
    src/backend.cpp \
    src/chirps.cpp \
    src/config.cpp \
    src/cursor.cpp \
    src/main.cpp \
    src/personality.cpp \
    src/pet.cpp \
    src/portalfilepicker.cpp \
    src/theme.cpp \
    src/translator.cpp \
    src/wakeword.cpp

RESOURCES += src/resources.qrc
