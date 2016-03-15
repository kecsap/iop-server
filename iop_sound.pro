TEMPLATE = app

QT += qml quick widgets multimedia

INCLUDEPATH += \
    /usr/include/libmindcommon \
    /usr/include/libmindaibo \
    /usr/include/libmindeye

LIBS += -lmindcommon -lmindaibo -lmindeye

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
