import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PlayerGui 1.0
import "components"

ApplicationWindow {
    id: root
    width: 1366
    height: 840
    visible: true
    title: "Aurora Player"
    color: "#E9EEF6"

    property bool fullscreen: false

    function formatTime(ms) {
        const safe = Math.max(0, Math.floor(ms / 1000))
        const hh = Math.floor(safe / 3600)
        const mm = Math.floor((safe % 3600) / 60)
        const ss = safe % 60
        if (hh > 0) {
            return hh + ":" + (mm < 10 ? "0" + mm : mm) + ":" + (ss < 10 ? "0" + ss : ss)
        }
        return mm + ":" + (ss < 10 ? "0" + ss : ss)
    }

    Shortcut {
        sequence: "Space"
        onActivated: {
            if (playerBridge.playbackState === PlayerBridge.Playing) playerBridge.pause()
            else playerBridge.play()
        }
    }
    Shortcut { sequence: "Left"; onActivated: playerBridge.rewind() }
    Shortcut { sequence: "Right"; onActivated: playerBridge.fastForward() }
    Shortcut { sequence: "Up"; onActivated: playerBridge.volume = Math.min(1.0, playerBridge.volume + 0.05) }
    Shortcut { sequence: "Down"; onActivated: playerBridge.volume = Math.max(0.0, playerBridge.volume - 0.05) }
    Shortcut {
        sequence: "F"
        onActivated: {
            root.fullscreen = !root.fullscreen
            root.visibility = root.fullscreen ? Window.FullScreen : Window.Windowed
            playerBridge.toggleFullscreen()
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#EFF4FB" }
            GradientStop { position: 0.45; color: "#DFE8F5" }
            GradientStop { position: 1.0; color: "#D6DDE9" }
        }
    }

    Rectangle {
        width: 520
        height: 520
        radius: 260
        color: "#66BCD6FF"
        x: -120
        y: -150
        opacity: 0.35
    }

    Rectangle {
        width: 600
        height: 600
        radius: 300
        color: "#55FFE7CC"
        x: root.width - width + 120
        y: root.height - height + 80
        opacity: 0.24
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        GlassCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 28
            tintColor: "#99FFFFFF"
            blurAmount: 0.58

            Item {
                anchors.fill: parent

                VideoRenderer {
                    id: videoStage
                    anchors.fill: parent

                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        border.width: 1
                        border.color: "#334A4A4F"
                    }

                    Text {
                        text: playerBridge.hasMedia ? "" : "Open a media file to start"
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        color: "#EAF2FF"
                        visible: !playerBridge.hasMedia
                        font.pixelSize: 20
                        font.family: "SF Pro Display, Segoe UI, Helvetica Neue, sans-serif"
                    }

                    Text {
                        text: playerBridge.hasMedia ? playerBridge.mediaTitle : "No media"
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 20
                        anchors.bottomMargin: 18
                        color: "#CCD8EA"
                        font.pixelSize: 14
                    }

                    Rectangle {
                        id: stateBadge
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 14
                        radius: 8
                        color: "#44728AAF"
                        implicitWidth: badgeText.implicitWidth + 16
                        implicitHeight: badgeText.implicitHeight + 10

                        Text {
                            id: badgeText
                            anchors.centerIn: parent
                            text: {
                                switch (playerBridge.playbackState) {
                                case PlayerBridge.Playing: return "PLAYING"
                                case PlayerBridge.Paused: return "PAUSED"
                                case PlayerBridge.Seeking: return "SEEKING"
                                case PlayerBridge.Stopped: return "STOPPED"
                                case PlayerBridge.Buffering: return "BUFFERING"
                                case PlayerBridge.Error: return "ERROR"
                                default: return "IDLE"
                                }
                            }
                            color: "#F1F8FF"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }
            }
        }

        GlassCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 206
            radius: 26
            tintColor: "#B8FFFFFF"
            blurAmount: 0.62

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: playerBridge.mediaTitle.length > 0 ? playerBridge.mediaTitle : "Untitled Media"
                            color: "#121318"
                            font.pixelSize: 24
                            font.bold: true
                            font.family: "SF Pro Display, Segoe UI, Helvetica Neue, sans-serif"
                            elide: Text.ElideRight
                        }

                        Text {
                            text: playerBridge.mediaArtist.length > 0 ? playerBridge.mediaArtist : "Unknown Artist"
                            color: "#525661"
                            font.pixelSize: 14
                            elide: Text.ElideRight
                        }
                    }

                    RowLayout {
                        spacing: 8

                        ControlButton {
                            text: "Open"
                            onClicked: openDialog.open()
                        }
                        ControlButton {
                            text: "Mode"
                            onClicked: playerBridge.cyclePlaybackMode()
                        }
                        ControlButton {
                            text: "x" + Number(playerBridge.playbackRate).toFixed(2)
                            onClicked: {
                                let next = playerBridge.playbackRate + 0.25
                                if (next > 2.0) next = 0.75
                                playerBridge.playbackRate = next
                            }
                        }
                    }
                }

                TimelineSlider {
                    id: timeline
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    from: 0
                    to: Math.max(1, playerBridge.durationMs)
                    value: playerBridge.positionMs
                    bufferedValue: playerBridge.bufferedRatio

                    onMoved: playerBridge.seekTo(value)
                }

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: formatTime(playerBridge.positionMs)
                        color: "#30343E"
                        font.pixelSize: 12
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: {
                            switch (playerBridge.playbackMode) {
                            case PlayerBridge.RepeatOne: return "Mode: Repeat One"
                            case PlayerBridge.RepeatAll: return "Mode: Repeat All"
                            case PlayerBridge.Shuffle: return "Mode: Shuffle"
                            default: return "Mode: Sequential"
                            }
                        }
                        color: "#485264"
                        font.pixelSize: 12
                    }

                    Item { Layout.preferredWidth: 12 }

                    Text {
                        text: formatTime(playerBridge.durationMs)
                        color: "#30343E"
                        font.pixelSize: 12
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ControlButton { text: "Prev"; onClicked: playerBridge.playPrevious() }
                    ControlButton { text: "-10s"; onClicked: playerBridge.rewind() }

                    ControlButton {
                        emphasized: true
                        implicitWidth: 66
                        text: playerBridge.playbackState === PlayerBridge.Playing ? "Pause" : "Play"
                        onClicked: {
                            if (playerBridge.playbackState === PlayerBridge.Playing) playerBridge.pause()
                            else playerBridge.play()
                        }
                    }

                    ControlButton { text: "+10s"; onClicked: playerBridge.fastForward() }
                    ControlButton { text: "Next"; onClicked: playerBridge.playNext() }
                    ControlButton { text: "Stop"; onClicked: playerBridge.stop() }

                    Item { Layout.fillWidth: true }

                    ControlButton {
                        text: playerBridge.muted ? "Unmute" : "Mute"
                        onClicked: playerBridge.toggleMute()
                    }

                    Slider {
                        id: volumeSlider
                        Layout.preferredWidth: 130
                        from: 0
                        to: 1
                        value: playerBridge.volume
                        onMoved: playerBridge.volume = value
                    }

                    ControlButton {
                        text: root.fullscreen ? "Window" : "Full"
                        onClicked: {
                            root.fullscreen = !root.fullscreen
                            root.visibility = root.fullscreen ? Window.FullScreen : Window.Windowed
                            playerBridge.toggleFullscreen()
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: openDialog
        title: "Open media"
        onAccepted: {
            if (selectedFile.toString().length > 0) {
                playerBridge.openMedia(selectedFile)
            }
        }
    }

    Connections {
        target: ffmpegEngine
        function onFrameReady(frame) {
            videoStage.updateFrame(frame)
        }
    }

    Connections {
        target: playerBridge

        function onEventError(errorCode, message) {
            console.warn("Player error", errorCode, message)
        }

        function onEventInfo(message) {
            console.log("Player info", message)
        }
    }
}
