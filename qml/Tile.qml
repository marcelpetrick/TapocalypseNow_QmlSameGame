import QtQuick

Item {
    id: tile

    property int r
    property int c
    property int colorIndex: -1 // -1 means empty cell
    property var colorPalette: []
    property bool interactive: true
    property bool animatePosition: false
    property int moveDuration: 260
    // animation triggers from GameBoard
    property bool popActive: false
    property bool shakeActive: false
    property bool hintActive: false
    property int hintPulseEpoch: 0
    // safe color (also used for effects)
    readonly property color selectedColor: (colorIndex >= 0 && colorPalette.length > 0 && colorIndex < colorPalette.length) ? colorPalette[colorIndex] : "#222532"
    // track previous index to detect spawn after collapse
    property int _prevIndex: -2
    // jitter for shake
    property real jitterX: 0
    // spawn animation endpoints captured once in onColorIndexChanged
    property real _spawnFromY: 0
    property real _spawnToY: 0

    signal clicked()

    // visibility follows content
    visible: colorIndex >= 0
    // --- lifecycle cues -----------------------------------------------------
    // Spawn drop-in animation when this cell becomes non-empty
    onColorIndexChanged: {
        if (_prevIndex === -1 && colorIndex >= 0) {
            _spawnFromY = y - height * 0.25;
            _spawnToY   = y;
            spawnAnim.restart();
        }
        _prevIndex = colorIndex;
    }
    Component.onCompleted: {
        _prevIndex = colorIndex;
    }
    // --- pop preview (valid group) -----------------------------------------
    onPopActiveChanged: {
        if (popActive && colorIndex >= 0)
            popAnim.restart();

    }
    // --- shake reject (invalid) --------------------------------------------
    onShakeActiveChanged: {
        if (shakeActive)
            shakeAnim.restart();

    }
    onHintPulseEpochChanged: {
        if (hintActive && colorIndex >= 0)
            hintGlow.restart();

    }

    // Concentric translucent geometry preserves the neon halo without forcing
    // every tile through an offscreen layer and blur pass.
    Rectangle {
        anchors.fill: core
        anchors.margins: -3
        radius: core.radius + 3
        color: Qt.rgba(selectedColor.r, selectedColor.g, selectedColor.b, 0.24)
    }

    Rectangle {
        id: core

        anchors.fill: parent
        radius: width * 0.22
        border.width: 2
        border.color: "#1f2a6b"
        opacity: 0.98
        scale: pressedArea.pressed ? 0.94 : 1

        // quick flash overlay for pop
        Rectangle {
            id: flash

            anchors.fill: parent
            radius: core.radius
            color: selectedColor
            opacity: 0
        }

        gradient: Gradient {
            GradientStop {
                position: 0
                color: selectedColor
            }

            GradientStop {
                position: 1
                color: "#111525"
            }

        }

        // subtle hover/press
        Behavior on scale {
            NumberAnimation {
                duration: 90
                easing.type: Easing.OutQuad
            }

        }

    }

    // --- interactions -------------------------------------------------------
    MouseArea {
        id: pressedArea

        anchors.fill: parent
        enabled: tile.interactive
        hoverEnabled: true
        onClicked: tile.clicked()
        onEntered: hoverGlow.running = true
    }

    // hover ripple
    Rectangle {
        id: ripple

        anchors.centerIn: parent
        width: 0
        height: 0
        radius: width / 2
        color: selectedColor
        opacity: 0.2
        visible: tile.visible
    }

    Rectangle {
        id: hintRipple

        anchors.centerIn: parent
        width: 0
        height: 0
        radius: width / 2
        color: selectedColor
        opacity: 0
        visible: tile.visible && tile.hintActive
    }

    SequentialAnimation {
        id: hoverGlow

        running: false
        onFinished: {
            ripple.width = 0;
            ripple.height = 0;
            ripple.opacity = 0.2;
        }

        PropertyAnimation {
            target: ripple
            property: "width"
            to: tile.width * 1.2
            duration: 220
        }

        PropertyAnimation {
            target: ripple
            property: "height"
            to: tile.height * 1.2
            duration: 220
        }

        PropertyAnimation {
            target: ripple
            property: "opacity"
            to: 0
            duration: 200
        }

    }

    SequentialAnimation {
        id: hintGlow

        running: false
        onStarted: {
            hintRipple.width = 0;
            hintRipple.height = 0;
            hintRipple.opacity = 0.24;
        }
        onFinished: {
            hintRipple.width = 0;
            hintRipple.height = 0;
            hintRipple.opacity = 0;
        }

        ParallelAnimation {
            SequentialAnimation {
                PropertyAnimation {
                    target: hintRipple
                    property: "width"
                    to: tile.width * 1.2
                    duration: 500
                    easing.type: Easing.OutQuad
                }

                PauseAnimation {
                    duration: 500
                }

            }

            SequentialAnimation {
                PropertyAnimation {
                    target: hintRipple
                    property: "height"
                    to: tile.height * 1.2
                    duration: 500
                    easing.type: Easing.OutQuad
                }

                PauseAnimation {
                    duration: 500
                }

            }

            SequentialAnimation {
                PauseAnimation {
                    duration: 120
                }

                PropertyAnimation {
                    target: hintRipple
                    property: "opacity"
                    to: 0.34
                    duration: 180
                }

                PropertyAnimation {
                    target: hintRipple
                    property: "opacity"
                    to: 0
                    duration: 700
                    easing.type: Easing.InQuad
                }

            }

        }

    }

    SequentialAnimation {
        id: spawnAnim

        running: false

        PropertyAnimation {
            target: tile
            property: "y"
            from: tile._spawnFromY
            to: tile._spawnToY
            duration: 180
            easing.type: Easing.OutQuad
        }

        PropertyAnimation {
            target: core
            property: "scale"
            from: 0.92
            to: 1
            duration: 120
            easing.type: Easing.OutBack
        }

        PropertyAnimation {
            target: core
            property: "opacity"
            from: 0
            to: 0.98
            duration: 160
        }

    }

    ParallelAnimation {
        id: popAnim

        running: false

        // scale pulse
        SequentialAnimation {
            PropertyAnimation {
                target: core
                property: "scale"
                to: 1.15
                duration: 110
                easing.type: Easing.OutCubic
            }

            PropertyAnimation {
                target: core
                property: "scale"
                to: 1
                duration: 90
                easing.type: Easing.InCubic
            }

        }

        // neon flash
        SequentialAnimation {
            PropertyAnimation {
                target: flash
                property: "opacity"
                to: 0.65
                duration: 90
            }

            PropertyAnimation {
                target: flash
                property: "opacity"
                to: 0
                duration: 120
            }

        }

    }

    SequentialAnimation {
        id: shakeAnim

        running: false

        // back-and-forth jitter without Keyframe*
        PropertyAnimation {
            target: tile
            property: "jitterX"
            to: -6
            duration: 50
            easing.type: Easing.OutQuad
        }

        PropertyAnimation {
            target: tile
            property: "jitterX"
            to: 6
            duration: 80
            easing.type: Easing.InOutQuad
        }

        PropertyAnimation {
            target: tile
            property: "jitterX"
            to: -4
            duration: 60
            easing.type: Easing.OutQuad
        }

        PropertyAnimation {
            target: tile
            property: "jitterX"
            to: 4
            duration: 60
            easing.type: Easing.InOutQuad
        }

        PropertyAnimation {
            target: tile
            property: "jitterX"
            to: 0
            duration: 50
            easing.type: Easing.OutQuad
        }

    }

    transform: Translate {
        x: tile.jitterX
    }

    Behavior on x {
        enabled: tile.animatePosition

        NumberAnimation {
            duration: tile.moveDuration
            easing.type: Easing.InOutCubic
        }

    }

    Behavior on y {
        enabled: tile.animatePosition

        NumberAnimation {
            duration: tile.moveDuration
            easing.type: Easing.InOutCubic
        }

    }

    NumberAnimation on opacity {
        // initial subtle fade on creation
        from: 0
        to: 1
        running: true
        duration: 180
        easing.type: Easing.OutQuad
    }

}
