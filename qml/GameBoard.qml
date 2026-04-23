import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    // keys "r,c" -> true briefly on invalid click

    id: board

    property int rows: 10
    property int cols: 16
    readonly property real cell: Math.min(width / cols, height / rows)
    property int score: 0
    property int moves: 0
    property bool gameOver: false
    property bool busy: false // block clicks while animating
    property int minimumGroupSize: 3
    // Internal model: -1 empty; >=0 color index
    property var grid: []
    // Selection maps for tile-level animations
    property var popSet: ({
    })
    // keys "r,c" -> true during valid pop preview
    property var shakeSet: ({
    })
    // ---- timers for animation staging -------------------------------------
    property var pendingRemoval: ({
    })
    property int pendingRow: -1
    property int pendingCol: -1

    signal gameEnded(int score)

    function neonPalette() {
        return ["#00E5FF", "#FF00E5", "#00FFAE", "#FFD400", "#8B80FF", "#FF5E5B"];
    }

    // ---- helpers -----------------------------------------------------------
    function inBounds(r, c) {
        return r >= 0 && r < rows && c >= 0 && c < cols;
    }

    function rebuildDelegates() {
        const m = rows * cols;
        if (repeater.model === m)
            repeater.model = 0;

        repeater.model = m;
    }

    function refreshGrid() {
        tileView.forceLayout();
    }

    function clearSelections() {
        popSet = ({
        });
        shakeSet = ({
        });
    }

    // ---- game logic --------------------------------------------------------
    function reset() {
        console.log("[GameBoard] reset()");
        clearSelections();
        busy = false;
        score = 0;
        moves = 0;
        gameOver = false;
        const paletteLen = neonPalette().length;
        let attempts = 0;
        let newGrid = [];
        do {
            newGrid = [];
            for (let r = 0; r < rows; ++r) {
                let row = new Array(cols);
                for (let c = 0; c < cols; ++c) row[c] = Math.floor(Math.random() * paletteLen)
                newGrid.push(row);
            }
            attempts++;
        } while (!gameLogic.hasMoves(newGrid, minimumGroupSize) && attempts < 128)
        pendingRow = -1;
        pendingCol = -1;
        pendingRemoval = ({
        });
        grid = newGrid;
        gameOver = !gameLogic.hasMoves(grid, minimumGroupSize);
        console.log("[GameBoard] reset() -> grid assigned, size:", rows, "x", cols, "attempts:", attempts);
        rebuildDelegates();
    }

    function removeGroup(r, c) {
        if (!inBounds(r, c))
            return ;

        if (gameOver) {
            console.log("[GameBoard] click ignored: game over");
            return ;
        }
        if (busy) {
            console.log("[GameBoard] click ignored: busy");
            return ;
        }
        const color = grid[r][c];
        if (color === -1) {
            console.log("[GameBoard] click on empty");
            return ;
        }
        const analysis = gameLogic.analyzeSelection(grid, r, c, minimumGroupSize);
        console.log("[GameBoard] click r:", r, "c:", c, "color:", color, "size:", analysis.groupSize);
        if (!analysis.valid) {
            shakeSet = ({
            });
            shakeSet[r + "," + c] = true;
            console.log("[GameBoard] shake single tile");
            shakeClearTimer.restart();
            return ;
        }
        busy = true;
        pendingRow = r;
        pendingCol = c;
        pendingRemoval = analysis.selection;
        popSet = analysis.selection;
        console.log("[GameBoard] pop preview for", Object.keys(popSet).length, "tiles");
        removeTimer.restart();
    }

    function doRemoveAndCollapse() {
        const move = gameLogic.applyMove(grid, pendingRow, pendingCol, minimumGroupSize);
        if (!move.valid) {
            console.log("[GameBoard] pending move became invalid");
            return ;
        }
        grid = move.grid;
        score += move.removedCount * move.removedCount;
        moves += 1;
        console.log("[GameBoard] removed", move.removedCount, "tiles -> score:", score, "moves:", moves);
        rebuildDelegates();
        refreshGrid();
        if (!move.hasMoves) {
            console.log("[GameBoard] no more moves -> game over, score:", score);
            gameOver = true;
            gameEnded(score);
        }
    }

    // Defer so delegates see an initialized grid
    Component.onCompleted: Qt.callLater(reset)

    Timer {
        id: removeTimer

        interval: 220 // pop preview duration
        repeat: false
        onTriggered: {
            console.log("[GameBoard] removeTimer fired");
            doRemoveAndCollapse();
            pendingRemoval = ({
            });
            pendingRow = -1;
            pendingCol = -1;
            popSet = ({
            });
            busy = false;
        }
    }

    Timer {
        id: shakeClearTimer

        interval: 180
        repeat: false
        onTriggered: {
            console.log("[GameBoard] shakeClearTimer fired");
            shakeSet = ({
            });
        }
    }

    // ---- view --------------------------------------------------------------
    Grid {
        id: tileView

        anchors.centerIn: parent
        columns: cols
        rows: board.rows
        spacing: cell * 0.06

        Repeater {
            id: repeater

            model: rows * cols

            delegate: Tile {
                width: cell
                height: cell
                r: Math.floor(index / cols)
                c: index % cols
                // Robust guard against uninitialized grid access
                colorIndex: (board.grid[r] !== undefined && board.grid[r][c] !== undefined) ? board.grid[r][c] : -1
                palette: board.neonPalette()
                // animation flags from board maps
                popActive: board.popSet[r + "," + c] === true
                shakeActive: board.shakeSet[r + "," + c] === true
                onClicked: {
                    console.log("[Tile] clicked r:", r, "c:", c, "color:", colorIndex);
                    board.removeGroup(r, c);
                }
            }

        }

    }

}
