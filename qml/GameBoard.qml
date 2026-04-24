import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Tapocalypse 1.0

Item {
    id: board

    property int rows: 10
    property int cols: 16
    property int minimumGroupSize: 3
    readonly property real cell: Math.min(width / cols, height / rows)
    readonly property real spacing: cell * 0.06
    readonly property real pitch: cell + spacing
    readonly property int score: controller.score
    readonly property int moves: controller.moves
    readonly property bool gameOver: controller.gameOver
    readonly property bool busy: controller.busy

    signal gameEnded(int score)

    function neonPalette() {
        return ["#00E5FF", "#FF00E5", "#00FFAE", "#FFD400", "#8B80FF", "#FF5E5B"];
    }

    function reset() {
        controller.reset();
    }

    function registerUserAction() {
        controller.registerUserAction();
    }

    Component.onCompleted: Qt.callLater(controller.reset)

    GameBoardController {
        id: controller

        rows: board.rows
        cols: board.cols
        minimumGroupSize: board.minimumGroupSize
        onGameEnded: function(finalScore) {
            board.gameEnded(finalScore);
        }
    }

    Item {
        id: tileView

        anchors.centerIn: parent
        width: cols * cell + (cols - 1) * spacing
        height: rows * cell + (rows - 1) * spacing

        Repeater {
            id: repeater

            model: rows * cols

            delegate: Tile {
                width: cell
                height: cell
                r: Math.floor(index / cols)
                c: index % cols
                x: c * pitch
                y: r * pitch
                colorIndex: controller.hiddenSet[r + "," + c] === true ? -1 : ((controller.grid[r] !== undefined && controller.grid[r][c] !== undefined) ? controller.grid[r][c] : -1)
                palette: board.neonPalette()
                popActive: controller.popSet[r + "," + c] === true
                shakeActive: controller.shakeSet[r + "," + c] === true
                hintActive: controller.hintSet[r + "," + c] === true
                hintPulseEpoch: controller.hintPulseEpoch
                onClicked: {
                    controller.removeGroup(r, c);
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
                palette: board.neonPalette()
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
