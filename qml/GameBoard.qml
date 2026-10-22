import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Tapocalypse 1.0

Item {
    id: board

    property int rows: 10
    property int cols: 16
    property int minimumGroupSize: 3
    // Gaps are a fixed share of a cell, so solve for the cell that fits cells plus gaps on both axes.
    readonly property real gapRatio: 0.06
    readonly property real cell: Math.min(width / (cols + gapRatio * (cols - 1)), height / (rows + gapRatio * (rows - 1)))
    readonly property real spacing: cell * gapRatio
    readonly property real pitch: cell + spacing
    // Footprint of the tile grid, so the window can frame the grid instead of the whole area.
    readonly property real gridWidth: cols * cell + (cols - 1) * spacing
    readonly property real gridHeight: rows * cell + (rows - 1) * spacing
    readonly property int score: controller.score
    readonly property int moves: controller.moves
    readonly property bool gameOver: controller.gameOver
    readonly property bool busy: controller.busy
    // Converted to JS once per change; the tile delegates read these instead of copying
    // the controller's QVariant containers once per tile and per property.
    readonly property var grid: controller.grid
    readonly property var popSet: controller.popSet
    readonly property var shakeSet: controller.shakeSet
    readonly property var hiddenSet: controller.hiddenSet
    readonly property var hintSet: controller.hintSet
    readonly property var hoverSet: controller.hoverSet
    readonly property var tilePalette: ["#00E5FF", "#FF00E5", "#00FFAE", "#FFD400", "#8B80FF", "#FF5E5B"]
    // Incremented whenever a fresh board is dealt so tiles can play their intro.
    property int dealEpoch: 0
    // Cue player (SoundEffects); null keeps the board silent.
    property QtObject sounds: null

    signal gameEnded(int score)

    function reset() {
        controller.reset();
        dealEpoch++;
        if (sounds)
            sounds.playDeal();

    }

    function endRun() {
        controller.endRun();
    }

    function registerUserAction() {
        controller.registerUserAction();
    }

    Component.onCompleted: Qt.callLater(board.reset)

    GameBoardController {
        id: controller

        objectName: "controller"
        rows: board.rows
        cols: board.cols
        minimumGroupSize: board.minimumGroupSize
        onGameEnded: function(finalScore) {
            if (board.sounds)
                board.sounds.playGameOver();

            board.gameEnded(finalScore);
        }
        onGroupSelected: function(size) {
            if (board.sounds)
                board.sounds.playPop(size);

        }
        onMoveRejected: {
            if (board.sounds)
                board.sounds.playReject();

        }
        onTilesLanded: {
            if (board.sounds)
                board.sounds.playLand();

        }
    }

    Timer {
        interval: 25
        repeat: true
        running: Qt.application.arguments.indexOf("--benchmark") >= 0
        triggeredOnStart: true
        onTriggered: controller.benchmarkStep()
    }

    Item {
        id: tileView

        objectName: "tileView"
        anchors.centerIn: parent
        width: board.gridWidth
        height: board.gridHeight

        Repeater {
            id: repeater

            model: rows * cols

            delegate: Tile {
                readonly property string key: r + "," + c
                readonly property var gridRow: board.grid[r]

                width: cell
                height: cell
                r: Math.floor(index / cols)
                c: index % cols
                x: c * pitch
                y: r * pitch
                colorIndex: board.hiddenSet[key] === true ? -1 : ((gridRow !== undefined && gridRow[c] !== undefined) ? gridRow[c] : -1)
                colorPalette: board.tilePalette
                popActive: board.popSet[key] === true
                shakeActive: board.shakeSet[key] === true
                hintActive: board.hintSet[key] === true
                hoverActive: board.hoverSet[key] === true
                hintPulseEpoch: controller.hintPulseEpoch
                spawnEpoch: board.dealEpoch
                onClicked: {
                    controller.removeGroup(r, c);
                }
                onHovered: function(inside) {
                    if (inside)
                        controller.setHoverCell(r, c);
                    else
                        controller.leaveHoverCell(r, c);
                }
            }

        }

        Repeater {
            model: controller.movingTiles

            delegate: Tile {
                required property var modelData

                width: cell
                height: cell
                colorIndex: modelData.color
                colorPalette: board.tilePalette
                interactive: false
                animatePosition: true
                moveDuration: modelData.duration
                x: modelData.fromCol * pitch
                y: modelData.fromRow * pitch
                z: 10
                Component.onCompleted: {
                    x = modelData.toCol * pitch;
                    y = modelData.toRow * pitch;
                }
            }

        }

    }

}
