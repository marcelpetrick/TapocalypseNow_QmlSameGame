import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Tapocalypse 1.0

ApplicationWindow {
    id: root

    property int gridCols: 16
    property int gridRows: 10
    // Full screen is the default (kiosk / embedded targets); --windowed opts out for desktop use.
    readonly property bool windowed: Qt.application.arguments.indexOf("--windowed") >= 0
    // One scale unit drives every size, so the HUD and dialogs follow the screen.
    readonly property real unit: Math.max(12, Math.min(width / 64, height / 36))

    // Start at the screen size so the first layout pass is already correct; in windowed mode
    // use a comfortable share of it. Tile positions stay bound, so later resizes reflow too.
    width: windowed ? Math.min(1280, Screen.desktopAvailableWidth) : Screen.width
    height: windowed ? Math.min(800, Screen.desktopAvailableHeight) : Screen.height
    minimumWidth: 480
    minimumHeight: 360
    visibility: windowed ? Window.Windowed : Window.FullScreen
    visible: true
    title: "Tapocalypse"
    color: "#07080F" // deep space

    TapHandler {
        acceptedButtons: Qt.AllButtons
        onTapped: game.registerUserAction()
    }

    // Futuristic neon header
    Rectangle {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: hud.height + 2 * root.unit
        opacity: 0.12

        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#0ef"
            }

            GradientStop {
                position: 0.5
                color: "#7a00ff"
            }

            GradientStop {
                position: 1
                color: "#00ff9f"
            }

        }

    }

    // HUD: score on the left, actions on the right; wraps onto two rows on narrow screens.
    Flow {
        id: hud

        anchors.top: parent.top
        anchors.topMargin: root.unit
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.unit * 1.5
        anchors.rightMargin: root.unit * 1.5
        spacing: root.unit

        Row {
            id: stats

            spacing: root.unit * 1.5

            Label {
                text: "Score: " + game.score
                font.pixelSize: root.unit * 1.6
                color: "#7dfcff"
            }

            Label {
                text: "Moves: " + game.moves
                font.pixelSize: root.unit * 1.6
                color: "#d0a2ff"
            }

            Label {
                text: "Game Over"
                font.pixelSize: root.unit * 1.6
                color: "#ff7bd5"
                visible: game.gameOver
            }

        }

        // Pushes the buttons to the right edge while there is room on the first row.
        Item {
            width: Math.max(0, hud.width - stats.width - actions.width - 2 * hud.spacing - 1)
            height: 1
        }

        Row {
            id: actions

            spacing: root.unit * 0.5

            NeonButton {
                objectName: "newGameButton"
                text: "New Game"
                font.pixelSize: root.unit
                onClicked: {
                    game.registerUserAction();
                    game.reset();
                }
            }

            NeonButton {
                objectName: "endRunButton"
                accent: "#ff7bd5"
                text: "End Run"
                font.pixelSize: root.unit
                enabled: !game.gameOver && !game.busy && game.moves > 0
                onClicked: {
                    game.registerUserAction();
                    game.endRun();
                }
            }

            NeonButton {
                objectName: "highscoresButton"
                accent: "#d0a2ff"
                text: "Highscores"
                font.pixelSize: root.unit
                onClicked: {
                    game.registerUserAction();
                    scoreDialog.openForBrowsing();
                }
            }

        }

    }

    // Board container with glow lines
    Rectangle {
        anchors.fill: boardFrame
        anchors.margins: -root.unit * 0.3
        radius: boardFrame.radius + root.unit * 0.3
        color: "#285500ea"
    }

    Rectangle {
        id: boardFrame

        objectName: "boardFrame"
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: root.unit * 1.5
        radius: root.unit * 1.2
        color: "#0B0D19"
        border.color: "#14204F"
        border.width: 2

        // GameBoard fits the largest square cell into both dimensions and centres the grid.
        GameBoard {
            id: game

            objectName: "gameBoard"
            anchors.fill: parent
            anchors.margins: root.unit
            rows: root.gridRows
            cols: root.gridCols
            onGameEnded: function(finalScore) {
                scoreDialog.consider(finalScore);
            }
        }

    }

    ScoreDialog {
        id: scoreDialog

        objectName: "scoreDialog"
        anchors.centerIn: parent
        unit: root.unit
        highScoreService: highScoreManager
    }

}
