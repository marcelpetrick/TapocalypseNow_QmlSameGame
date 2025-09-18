import QtQuick
import QtQuick.Effects

Item {
    id: tile
    property int r
    property int c
    property int colorIndex: -1             // -1 means empty cell
    property var  palette: []

    // animation triggers from GameBoard
    property bool popActive: false
    property bool shakeActive: false

    // visibility follows content
    visible: colorIndex >= 0

    // safe color (also used for effects)
    readonly property color selectedColor:
        (colorIndex >= 0 && palette.length > 0 && colorIndex < palette.length)
            ? palette[colorIndex] : "#222532"

    // track previous index to detect spawn after collapse
    property int _prevIndex: -2

    // jitter for shake
    property real jitterX: 0
    transform: Translate { x: tile.jitterX }

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
        opacity: 0.98
        scale: pressedArea.pressed ? 0.94 : 1.0

        // subtle hover/press
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

        // quick flash overlay for pop
        Rectangle {
            id: flash
            anchors.fill: parent
            radius: core.radius
            color: selectedColor
            opacity: 0.0
        }
    }

    // --- interactions -------------------------------------------------------
    MouseArea {
        id: pressedArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: tile.clicked()
        onEntered: hoverGlow.running = true
    }
    signal clicked()

    // hover ripple
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
        PropertyAnimation { target: ripple; property: "width";  to: tile.width * 1.2; duration: 220 }
        PropertyAnimation { target: ripple; property: "height"; to: tile.height * 1.2; duration: 220 }
        PropertyAnimation { target: ripple; property: "opacity"; to: 0.0; duration: 200 }
        onFinished: { ripple.width = 0; ripple.height = 0; ripple.opacity = 0.2 }
    }

    // --- lifecycle cues -----------------------------------------------------
    // Spawn drop-in animation when this cell becomes non-empty
    onColorIndexChanged: {
        if (_prevIndex === -1 && colorIndex >= 0) {
            spawnAnim.restart()
        }
        _prevIndex = colorIndex
    }
    Component.onCompleted: { _prevIndex = colorIndex }

    NumberAnimation on opacity {
        // initial subtle fade on creation
        from: 0.0; to: 1.0; running: true; duration: 180; easing.type: Easing.OutQuad
    }
    SequentialAnimation {
        id: spawnAnim
        PropertyAnimation { target: tile; property: "y"; from: tile.y - tile.height * 0.25; to: tile.y; duration: 180; easing.type: Easing.OutQuad }
        PropertyAnimation { target: core; property: "scale"; from: 0.92; to: 1.0; duration: 120; easing.type: Easing.OutBack }
        PropertyAnimation { target: core; property: "opacity"; from: 0.0; to: 0.98; duration: 160 }
        running: false
    }

    // --- pop preview (valid group) -----------------------------------------
    onPopActiveChanged: if (popActive && colorIndex >= 0) popAnim.restart()
    ParallelAnimation {
        id: popAnim
        running: false
        // scale pulse
        SequentialAnimation {
            PropertyAnimation { target: core; property: "scale"; to: 1.15; duration: 110; easing.type: Easing.OutCubic }
            PropertyAnimation { target: core; property: "scale"; to: 1.0;  duration: 90;  easing.type: Easing.InCubic }
        }
        // neon flash
        SequentialAnimation {
            PropertyAnimation { target: flash; property: "opacity"; to: 0.65; duration: 90 }
            PropertyAnimation { target: flash; property: "opacity"; to: 0.0;  duration: 120 }
        }
    }

    // --- shake reject (invalid) --------------------------------------------
    onShakeActiveChanged: if (shakeActive) shakeAnim.restart()
    SequentialAnimation {
        id: shakeAnim
        running: false
        // back-and-forth jitter without Keyframe*
        PropertyAnimation { target: tile; property: "jitterX"; to: -6; duration: 50;  easing.type: Easing.OutQuad }
        PropertyAnimation { target: tile; property: "jitterX"; to:  6; duration: 80;  easing.type: Easing.InOutQuad }
        PropertyAnimation { target: tile; property: "jitterX"; to: -4; duration: 60;  easing.type: Easing.OutQuad }
        PropertyAnimation { target: tile; property: "jitterX"; to:  4; duration: 60;  easing.type: Easing.InOutQuad }
        PropertyAnimation { target: tile; property: "jitterX"; to:  0; duration: 50;  easing.type: Easing.OutQuad }
    }
}
