
TEMPLATE = app

QT += qml quick widgets multimedia concurrent

INCLUDEPATH += \
    /usr/include/libmindcommon \
    /usr/include/libmindaibo \
    /usr/include/libmindeye \
    /usr/include/libmindsession \
    /usr/include/libmindpiece

LIBS += -lmindcommon -lmindaibo -lmindeye -lmindsession -lmindpiece -lopencv_core -lopencv_imgproc

SOURCES += \
    main.cpp \
    AudioWatcher.cpp \
    GameWatcher.cpp \
    ImageSender.cpp \
    TableMarkers.cpp \
    VideoWatcher.cpp

HEADERS += \
    AudioWatcher.hpp \
    GameWatcher.hpp \
    ImageSender.hpp \
    TableMarkers.hpp \
    VideoWatcher.hpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

DISTFILES +=
