
TEMPLATE = app

# Add detailed debug information
#QMAKE_CXXFLAGS += -ggdb3

# Clang's AddressSanitizer for lightweight debugging
#QMAKE_CXX = clang++
#QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer
#QMAKE_LINK = clang++
#QMAKE_LFLAGS += -fsanitize=address

QT += qml quick widgets multimedia concurrent

INCLUDEPATH += \
    /usr/include/libmindcommon \
    /usr/include/libmindaibo \
    /usr/include/libmindeye \
    /usr/include/libmindsession \
    /usr/include/libmindpiece

LIBS += -lmindcommon -lmindaibo -lmindeye -lmindsession -lopencv_core -lopencv_imgproc

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
