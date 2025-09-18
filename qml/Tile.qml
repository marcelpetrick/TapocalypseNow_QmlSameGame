import QtQuick 2.15
import Qt5Compat.GraphicalEffects


Item {
    id: tile
    property int r
    property int c
    property int colorIndex: -1
    property var palette: []
    signal clicked()


    visible: colorIndex >= 0


    Rectangle {
        id: core
        anchors.fill: parent
        radius: width * 0.22
        gradient: Gradient {
            GradientStop { position: 0.0; color: palette[colorIndex] }
            GradientStop { position: 1.0; color: "#111525" }
        }
        border.width: 2
        border.color: "#1f2a6b"
        opacity: 0.94
        scale: pressedArea.pressed ? 0.94 : 1.0
        Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }


        layer.enabled: true
        layer.effect: DropShadow {
            radius: 18
            samples: 24
            horizontalOffset: 0
            verticalOffset: 0
            color: palette[colorIndex]
        }
    }


    // subtle pulse
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
        radius: width/2
        color: palette[colorIndex]
        opacity: 0.20
    }
    SequentialAnimation {
        id: hoverGlow
        running: false
        PropertyAnimation { target: ripple; property: "width"; to: tile.width*1.2; duration: 220 }
        PropertyAnimation { target: ripple; property: "height"; to: tile.height*1.2; duration: 220 }
        PropertyAnimation { target: ripple; property: "opacity"; to: 0.0; duration: 200 }
        onFinished: { ripple.width = 0; ripple.height = 0; ripple.opacity = 0.2 }
    }
}
