import QtQuick 2.0
import QtQuick.Window 2.0

Window {
    visible: true
Rectangle{
    id: fuckYoo
    objectName: "fuckYoo"
    anchors.fill: parent

    property alias resultText: resultLabel.text

    MouseArea {
        anchors.fill: parent
        onClicked: {
            Qt.quit();
        }
    }


    Text {
        id: resultLabel
        font.pixelSize: 100
        text: ""
        anchors.centerIn: parent
    }
}

}