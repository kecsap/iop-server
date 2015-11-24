TEMPLATE = app

QT += qml quick multimedia

LIBS += -lopencv_core -lopencv_ml

DEFINES += USE_OOURA

INCLUDEPATH += libs/libxtract

QMAKE_CFLAGS += -Wno-unused-parameter -std=c99

SOURCES += \
    libs/libxtract/c-ringbuf/ringbuf.c \
    libs/libxtract/dywapitchtrack/dywapitchtrack.c \
    libs/libxtract/ooura/fftsg.c \
    libs/libxtract/delta.c \
    libs/libxtract/descriptors.c \
    libs/libxtract/fini.c \
    libs/libxtract/helper.c \
    libs/libxtract/init.c \
    libs/libxtract/libxtract.c \
    libs/libxtract/scalar.c \
    libs/libxtract/stateful.c \
    libs/libxtract/vector.c \
    libs/libxtract/window.c

SOURCES += \
    main.cpp \
    SoundRecorder.cpp \

HEADERS += \
    SoundRecorder.hpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

DISTFILES +=

