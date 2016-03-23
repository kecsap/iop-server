
PROJECT(IOP_SERVER)

SET(CMAKE_AUTOMOC ON)

INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5/Qt5Config.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Core/Qt5CoreConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Network/Qt5NetworkConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Gui/Qt5GuiConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Qml/Qt5QmlConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Quick/Qt5QuickConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Multimedia/Qt5MultimediaConfig.cmake)

INCLUDE_DIRECTORIES(/usr/include/libmindcommon /usr/include/libmindaibo /usr/include/libmindeye)
INCLUDE_DIRECTORIES(/usr/include/libmindsession /usr/include/libmindpiece)

SET(IOP_SERVER_SRC
    main.cpp ;
    AudioWatcher.cpp ;
    ImageSender.cpp ;
    TableMarkers.cpp ;
    VideoWatcher.cpp ;
    GameWatcher.cpp)

SET(IOP_SERVER_HEADERS
    AudioWatcher.hpp ;
    ImageSender.hpp ;
    VideoWatcher.hpp ;
    TableMarkers.hpp ;
    GameWatcher.hpp)

QT5_ADD_RESOURCES(IOP_SERVER_RCC_SRC qml.qrc)

ADD_EXECUTABLE(iop-sound ${IOP_SERVER_SRC} ${IOP_SERVER_RCC_SRC} ${IOP_SERVER_HEADERS})
TARGET_LINK_LIBRARIES(iop-sound Qt5::Core Qt5::Qml Qt5::Quick Qt5::Multimedia Qt5::Concurrent
                      -lmindcommon -lmindaibo -lmindeye -lmindsession -lmindpiece)
