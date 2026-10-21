import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tapocalypse 1.0

ApplicationWindow {
    id: root

    property int gridCols: 16
    property int gridRows: 10

    width: 1920
    height: 1080
    visibility: Window.FullScreen
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
        height: 88
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

    // HUD
    RowLayout {
        anchors.top: parent.top
        anchors.topMargin: 18
        anchors.left: parent.left
        anchors.leftMargin: 24
        spacing: 24

        Label {
            text: "Score: " + game.score
            font.pixelSize: 28
            color: "#7dfcff"
        }

        Label {
            text: "Moves: " + game.moves
            font.pixelSize: 28
            color: "#d0a2ff"
        }

        Label {
            text: game.gameOver ? "Game Over" : ""
            font.pixelSize: 28
            color: "#ff7bd5"
            visible: game.gameOver
        }

        Item {
            Layout.fillWidth: true
        }

        Button {
            text: "New Game"
            onClicked: {
                game.registerUserAction();
                console.log("[Main] New Game clicked");
                game.reset();
            }
        }

        Button {
            text: "Highscores"
            onClicked: {
                game.registerUserAction();
                scoreDialog.openForBrowsing();
            }
        }

    }

    // Board container with glow lines
    Rectangle {
        anchors.fill: boardFrame
        anchors.margins: -6
        radius: boardFrame.radius + 6
        color: "#285500ea"
    }

    Rectangle {
        id: boardFrame

        anchors.fill: parent
        anchors.margins: 120
        radius: 24
        color: "#0B0D19"
        border.color: "#14204F"
        border.width: 2

        GameBoard {
            id: game

            // aspect = cols / rows (how many tiles wide vs tall)
            readonly property real aspect: cols / rows

            anchors.centerIn: parent
            // shrink/grow here: 0.55 = 55% of frame width
            width: boardFrame.width * 0.55
            height: width / aspect
            rows: root.gridRows
            cols: root.gridCols
            onGameEnded: function(finalScore) {
                scoreDialog.consider(finalScore);
            }
        }

    }

    ScoreDialog {
        id: scoreDialog

        anchors.centerIn: parent
        highScoreService: highScoreManager
    }

}
