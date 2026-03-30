import QtQuick
import QtQuick.Effects

Item {
    id: root

    property alias contentItem: contentContainer
    property real radius: 22
    property color tintColor: "#B3FFFFFF"
    property real shadowOpacity: 0.22
    property real blurAmount: 0.55

    default property alias data: contentContainer.data

    Rectangle {
        id: shadowLayer
        anchors.fill: panel
        anchors.margins: -8
        radius: root.radius + 8
        color: Qt.rgba(0, 0, 0, root.shadowOpacity)
        visible: true
    }

    Rectangle {
        id: panel
        anchors.fill: parent
        radius: root.radius
        color: root.tintColor
        border.color: "#66FFFFFF"
        border.width: 1
        clip: true
    }

    MultiEffect {
        anchors.fill: panel
        source: panel
        blurEnabled: true
        blur: root.blurAmount
        saturation: 0.12
        brightness: 0.02
    }

    Item {
        id: contentContainer
        anchors.fill: panel
        anchors.margins: 16
    }
}
