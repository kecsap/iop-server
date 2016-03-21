import QtQuick 2.0
import QtQuick.Window 2.0

Window {
    visible: true
    width: 640
    height: 360

Rectangle {
    id: fuckYoo
    objectName: "fuckYoo"

    property alias cameraImage: cameraImage.source

    anchors.fill: parent

    // Camera image
    Image {
        id: cameraImage
        anchors.fill: parent
        smooth: true
        visible: true
        source: "image://camera/image.png"
    }
}

}