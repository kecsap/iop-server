
PROJECT(IOPSOUND)

SET(CMAKE_AUTOMOC ON)

INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5/Qt5Config.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Core/Qt5CoreConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Network/Qt5NetworkConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Gui/Qt5GuiConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Qml/Qt5QmlConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Quick/Qt5QuickConfig.cmake)
INCLUDE(/usr/local/Qt-5.6.0/lib/cmake/Qt5Multimedia/Qt5MultimediaConfig.cmake)

INCLUDE_DIRECTORIES(/usr/include/libmindcommon /usr/include/libmindaibo /usr/include/libmindeye)

SET(IOPSOUND_SRC
    main.cpp ;
    SoundRecorder.cpp)

SET(IOPSOUND_HEADERS
    SoundRecorder.cpp)

QT5_ADD_RESOURCES(IOPSOUND_RCC_SRC qml.qrc)

ADD_EXECUTABLE(iop-sound ${IOPSOUND_SRC} ${IOPSOUND_RCC_SRC} ${IOPSOUND_HEADERS})
TARGET_LINK_LIBRARIES(iop-sound Qt5::Core Qt5::Qml Qt5::Quick Qt5::Multimedia -lmindcommon -lmindaibo -lmindeye)
