import QtQuick
import QtQuick.Controls

// Flat neon-outlined button matching the board theme.
Button {
    id: control

    property color accent: "#7dfcff"

    padding: font.pixelSize * 0.5
    leftPadding: font.pixelSize
    rightPadding: font.pixelSize

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? control.accent : "#4a5070"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitHeight: control.font.pixelSize * 2.2
        radius: height / 2
        color: control.down ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, 0.3) : control.hovered && control.enabled ? Qt.rgba(control.accent.r, control.accent.g, control.accent.b, 0.15) : "#0F1020"
        border.width: control.visualFocus ? 3 : 2
        border.color: control.enabled ? control.accent : "#262a40"

        Behavior on color {
            ColorAnimation {
                duration: 120
            }

        }

    }

}
