import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    // keys "r,c" -> true briefly on invalid click
    // Bindings update automatically; keep this helper so phase code reads clearly.

    id: board

    property int rows: 10
    property int cols: 16
    readonly property real cell: Math.min(width / cols, height / rows)
    property int score: 0
    property int moves: 0
    property bool gameOver: false
    property bool busy: false // block clicks while animating
    property int minimumGroupSize: 3
    readonly property real spacing: cell * 0.06
    readonly property real pitch: cell + spacing
    readonly property int removeHoldMs: 200
    readonly property int fallAnimMs: 320
    readonly property int shiftAnimMs: 260
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
    property bool phaseHasMoves: false
    property var hiddenSet: ({
    })
    property var movingTiles: []
    property var pendingMovePlan: ({
    })

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
    }

    function clearSelections() {
        popSet = ({
        });
        shakeSet = ({
        });
    }

    function clearPhaseState() {
        hiddenSet = ({
        });
        movingTiles = [];
        pendingMovePlan = ({
        });
        phaseHasMoves = false;
    }

    function hideMotionSources(motions) {
        let nextHidden = ({
        });
        for (let i = 0; i < motions.length; ++i) {
            const motion = motions[i];
            nextHidden[motion.fromRow + "," + motion.fromCol] = true;
        }
        hiddenSet = nextHidden;
    }

    function buildAnimatedTiles(motions, durationMs) {
        let nextTiles = [];
        for (let i = 0; i < motions.length; ++i) {
            const motion = motions[i];
            nextTiles.push({
                "color": motion.color,
                "fromRow": motion.fromRow,
                "fromCol": motion.fromCol,
                "toRow": motion.toRow,
                "toCol": motion.toCol,
                "duration": durationMs
            });
        }
        movingTiles = nextTiles;
    }

    function applyFallPhase() {
        if (!pendingMovePlan.valid) {
            finishMovePhases();
            return ;
        }
        if (pendingMovePlan.fallMotions.length === 0) {
            grid = pendingMovePlan.afterFallGrid;
            refreshGrid();
            applyShiftPhase();
            return ;
        }
        hideMotionSources(pendingMovePlan.fallMotions);
        buildAnimatedTiles(pendingMovePlan.fallMotions, fallAnimMs);
        fallTimer.restart();
    }

    function applyShiftPhase() {
        grid = pendingMovePlan.afterFallGrid;
        refreshGrid();
        if (pendingMovePlan.shiftMotions.length === 0) {
            finishMovePhases();
            return ;
        }
        hideMotionSources(pendingMovePlan.shiftMotions);
        buildAnimatedTiles(pendingMovePlan.shiftMotions, shiftAnimMs);
        shiftTimer.restart();
    }

    function finishMovePhases() {
        grid = pendingMovePlan.valid ? pendingMovePlan.afterShiftGrid : grid;
        clearSelections();
        clearPhaseState();
        pendingRemoval = ({
        });
        pendingRow = -1;
        pendingCol = -1;
        busy = false;
        if (!phaseHasMoves) {
            console.log("[GameBoard] no more moves -> game over, score:", score);
            gameOver = true;
            gameEnded(score);
        }
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
        clearPhaseState();
        grid = newGrid;
        gameOver = !gameLogic.hasMoves(grid, minimumGroupSize);
        console.log("[GameBoard] reset() -> grid assigned, size:", rows, "x", cols, "attempts:", attempts);
        refreshGrid();
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
        console.log("[GameBoard] selection valid:", analysis.valid ? "YES" : "NO");
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
        clearSelections();
        removeTimer.restart();
    }

    function startResolvedMove() {
        const move = gameLogic.planMove(grid, pendingRow, pendingCol, minimumGroupSize);
        if (!move.valid) {
            console.log("[GameBoard] pending move became invalid");
            busy = false;
            return ;
        }
        pendingMovePlan = move;
        phaseHasMoves = move.hasMoves;
        grid = move.afterRemovalGrid;
        score += move.removedCount * move.removedCount;
        moves += 1;
        console.log("[GameBoard] removed", move.removedCount, "tiles -> score:", score, "moves:", moves);
        refreshGrid();
        removeHoldTimer.restart();
    }

    // Defer so delegates see an initialized grid
    Component.onCompleted: Qt.callLater(reset)

    Timer {
        id: removeTimer

        interval: 220 // pop preview duration
        repeat: false
        onTriggered: {
            console.log("[GameBoard] removeTimer fired");
            startResolvedMove();
        }
    }

    Timer {
        id: removeHoldTimer

        interval: removeHoldMs
        repeat: false
        onTriggered: {
            console.log("[GameBoard] showing removal state for", removeHoldMs, "ms");
            applyFallPhase();
        }
    }

    Timer {
        id: fallTimer

        interval: fallAnimMs
        repeat: false
        onTriggered: {
            grid = pendingMovePlan.afterFallGrid;
            movingTiles = [];
            hiddenSet = ({
            });
            refreshGrid();
            applyShiftPhase();
        }
    }

    Timer {
        id: shiftTimer

        interval: shiftAnimMs
        repeat: false
        onTriggered: {
            movingTiles = [];
            hiddenSet = ({
            });
            finishMovePhases();
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
                // Robust guard against uninitialized grid access
                colorIndex: board.hiddenSet[r + "," + c] === true ? -1 : ((board.grid[r] !== undefined && board.grid[r][c] !== undefined) ? board.grid[r][c] : -1)
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

        Repeater {
            model: board.movingTiles

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
