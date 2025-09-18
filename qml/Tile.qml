import QtQuick
import QtQuick.Effects

Item {
    id: tile
    property int r
    property int c
    // If grid not ready, this is -1 -> tile hidden
    property int colorIndex: -1
    property var palette: []
    signal clicked()

    visible: colorIndex >= 0

    // pick a safe color even when hidden (avoids out-of-bounds)
    readonly property color selectedColor:
        (colorIndex >= 0 && palette.length > 0 && colorIndex < palette.length)
        ? palette[colorIndex] : "#222532"

    Rectangle {
        id: core
        anchors.fill: parent
        radius: width * 0.22
        gradient: Gradient {
            GradientStop { position: 0.0; color: selectedColor }
            GradientStop { position: 1.0; color: "#111525" }
        }
        border.width: 2
        border.color: "#1f2a6b"
        opacity: 0.94
        scale: pressedArea.pressed ? 0.94 : 1.0
        Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowBlur: 0.9
            shadowOpacity: 0.9
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 0
            shadowColor: selectedColor
        }
    }

    // subtle fade-in
    NumberAnimation on opacity {
        from: 0.0; to: 1.0; running: true; duration: 300; easing.type: Easing.OutQuad
    }

    MouseArea {
        id: pressedArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: tile.clicked()
        onEntered: hoverGlow.running = true
    }

    // hover glow ripple
    Rectangle {
        id: ripple
        anchors.centerIn: parent
        width: 0; height: 0
        radius: width / 2
        color: selectedColor
        opacity: 0.20
        visible: tile.visible
    }
    SequentialAnimation {
        id: hoverGlow
        running: false
        PropertyAnimation { target: ripple; property: "width"; to: tile.width * 1.2; duration: 220 }
        PropertyAnimation { target: ripple; property: "height"; to: tile.height * 1.2; duration: 220 }
        PropertyAnimation { target: ripple; property: "opacity"; to: 0.0; duration: 200 }
        onFinished: { ripple.width = 0; ripple.height = 0; ripple.opacity = 0.2 }
    }
}
