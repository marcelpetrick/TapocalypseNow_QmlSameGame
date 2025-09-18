import QtQuick 2.15
import QtQuick.Effects 1.15
import Qt5Compat.GraphicalEffects
import SameGame 1.0


ApplicationWindow {
id: root
width: 1920
height: 1080
visible: true
title: "SameGame — Neon"
color: "#07080F" // deep space


property int gridCols: 16
property int gridRows: 10


// Futuristic neon header
Rectangle {
id: header
anchors.top: parent.top
anchors.left: parent.left
anchors.right: parent.right
height: 88
gradient: Gradient {
GradientStop { position: 0; color: "#0ef" }
GradientStop { position: 0.5; color: "#7a00ff" }
GradientStop { position: 1; color: "#00ff9f" }
}
opacity: 0.12
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
Item { Layout.fillWidth: true }
Button {
text: "New Game"
onClicked: game.reset()
}
Button {
text: "Highscores"
onClicked: scoreDialog.openForBrowsing()
}
Button {
text: "Reset Highscores"
onClicked: highScoreManager.resetScores()
}
}


// Board container with glow lines
Rectangle {
id: boardFrame
anchors.fill: parent
anchors.margins: 40
radius: 24
color: "#0B0D19"
border.color: "#14204F"
border.width: 2
layer.enabled: true
layer.smooth: true
layer.effect: DropShadow {
radius: 32
samples: 32
color: "#5500eaff"
verticalOffset: 0
horizontalOffset: 0
}


GameBoard {
id: game
anchors.centerIn: parent
width: Math.min(parent.width - 200, parent.height - 220) * 1.6 // widescreen feel
height: width / 1.6
rows: root.gridRows
cols: root.gridCols
onGameEnded: scoreDialog.consider(score)
}
}


ScoreDialog {
id: scoreDialog
anchors.centerIn: parent
}
}
