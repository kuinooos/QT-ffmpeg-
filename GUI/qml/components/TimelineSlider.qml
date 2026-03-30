import QtQuick
import QtQuick.Controls

Slider {
    id: root

    property real bufferedValue: 0

    from: 0
    to: 1
    stepSize: 0

    background: Item {
        implicitHeight: 20

        Rectangle {
            id: rail
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: 6
            radius: 3
            color: "#55B8B8BD"
        }

        Rectangle {
            anchors.left: rail.left
            anchors.verticalCenter: rail.verticalCenter
            height: rail.height
            radius: rail.radius
            width: rail.width * Math.max(0, Math.min(1, root.bufferedValue))
            color: "#88C8D8F7"
        }

        Rectangle {
            anchors.left: rail.left
            anchors.verticalCenter: rail.verticalCenter
            height: rail.height
            radius: rail.radius
            width: rail.width * root.visualPosition
            color: "#0A84FF"
        }
    }

    handle: Rectangle {
        width: 16
        height: 16
        radius: 8
        color: root.pressed ? "#066EDD" : "white"
        border.width: 1
        border.color: "#99D0D0D2"
        y: (root.availableHeight - height) / 2
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)

        Behavior on color {
            ColorAnimation {
                duration: 100
            }
        }
    }
}
