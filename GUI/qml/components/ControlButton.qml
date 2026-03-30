import QtQuick
import QtQuick.Controls

Button {
    id: root

    property color accentColor: "#0A84FF"
    property bool emphasized: false

    implicitWidth: 44
    implicitHeight: 44

    background: Rectangle {
        radius: width / 2
        color: {
            if (!root.enabled) return "#4DF4F4F7"
            if (root.down) return root.emphasized ? "#006BDA" : "#D9E7FF"
            if (root.hovered) return root.emphasized ? "#1890FF" : "#EDF3FF"
            return root.emphasized ? root.accentColor : "#CCFFFFFF"
        }
        border.width: root.emphasized ? 0 : 1
        border.color: root.emphasized ? "transparent" : "#88D6D6D8"

        Behavior on color {
            ColorAnimation {
                duration: 120
            }
        }
    }

    contentItem: Text {
        text: root.text
        font.pixelSize: 13
        font.bold: root.emphasized
        font.family: "SF Pro Display, Segoe UI, Helvetica Neue, sans-serif"
        color: root.emphasized ? "white" : "#1E1E24"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
