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

    function cloneGrid(sourceGrid) {
        return sourceGrid.map((row) => {
            return row.slice();
        });
    }

    function makeEmptyGrid() {
        let newGrid = [];
        for (let r = 0; r < rows; ++r) {
            let row = new Array(cols);
            row.fill(-1);
            newGrid.push(row);
        }
        return newGrid;
    }

    // ---- game logic --------------------------------------------------------
    function reset() {
        console.log("[GameBoard] reset()");
        clearSelections();
        busy = false;
        score = 0;
        moves = 0;
        gameOver = false;
        // Build a NEW grid and assign once (triggers change notification)
        const paletteLen = neonPalette().length;
        let newGrid = [];
        for (let r = 0; r < rows; ++r) {
            let row = new Array(cols);
            for (let c = 0; c < cols; ++c) row[c] = Math.floor(Math.random() * paletteLen)
            newGrid.push(row);
        }
        grid = newGrid;
        console.log("[GameBoard] reset() -> grid assigned, size:", rows, "x", cols);
        rebuildDelegates();
    }

    function flood(r, c, color, visited) {
        if (!inBounds(r, c))
            return 0;

        const key = r + "," + c;
        if (visited[key])
            return 0;

        visited[key] = true;
        if (grid[r][c] !== color)
            return 0;

        let count = 1;
        count += flood(r + 1, c, color, visited);
        count += flood(r - 1, c, color, visited);
        count += flood(r, c + 1, color, visited);
        count += flood(r, c - 1, color, visited);
        return count;
    }

    function removeGroup(r, c) {
        if (!inBounds(r, c))
            return ;

        if (busy) {
            console.log("[GameBoard] click ignored: busy");
            return ;
        }
        const color = grid[r][c];
        if (color === -1) {
            console.log("[GameBoard] click on empty");
            return ;
        }
        const visited = {
        };
        const size = flood(r, c, color, visited);
        console.log("[GameBoard] click r:", r, "c:", c, "color:", color, "size:", size);
        if (size < minimumGroupSize) {
            // INVALID -> shake this single tile
            shakeSet = ({
            });
            shakeSet[r + "," + c] = true;
            console.log("[GameBoard] shake single tile");
            shakeClearTimer.restart();
            return ;
        }
        // VALID -> pop preview, then remove on timer
        busy = true;
        popSet = visited;
        console.log("[GameBoard] pop preview for", Object.keys(popSet).length, "tiles");
        pendingRemoval = visited;
        removeTimer.restart();
    }

    // Execute the actual removal + collapse after pop preview
    function doRemoveAndCollapse(selection) {
        // Work on a copy, then reassign to notify
        let g = cloneGrid(grid);
        // remove selected
        let removedCount = 0;
        for (let key in selection) {
            const [rr, cc] = key.split(",").map(Number);
            if (g[rr][cc] !== -1) {
                g[rr][cc] = -1;
                removedCount++;
            }
        }
        grid = g;
        // scoring & moves
        score += removedCount * removedCount;
        moves += 1;
        console.log("[GameBoard] removed", removedCount, "tiles -> score:", score, "moves:", moves);
        // collapse
        collapse();
        // end?
        if (!hasMoves()) {
            console.log("[GameBoard] no more moves -> game over, score:", score);
            gameOver = true;
            gameEnded(score);
        }
    }

    function collapse() {
        console.log("[GameBoard] collapse()");
        const sourceGrid = cloneGrid(grid);
        let collapsedGrid = makeEmptyGrid();
        let targetColumn = 0;
        for (let c = 0; c < cols; ++c) {
            let compactedColumn = [];
            for (let r = rows - 1; r >= 0; --r) {
                if (sourceGrid[r][c] !== -1)
                    compactedColumn.push(sourceGrid[r][c]);

            }
            if (compactedColumn.length === 0)
                continue;

            for (let i = 0; i < compactedColumn.length; ++i) collapsedGrid[rows - 1 - i][targetColumn] = compactedColumn[i]
            targetColumn++;
        }
        grid = collapsedGrid;
        console.log("[GameBoard] collapse() -> grid reassigned");
        rebuildDelegates(); // let tiles rebind for “drop-in” anim
        refreshGrid();
    }

    function hasMoves() {
        let visited = ({
        });
        for (let r = 0; r < rows; ++r) {
            for (let c = 0; c < cols; ++c) {
                const key = r + "," + c;
                const color = grid[r][c];
                if (color === -1 || visited[key] === true)
                    continue;

                const group = ({
                });
                const size = flood(r, c, color, group);
                for (let groupKey in group) visited[groupKey] = true
                if (size >= minimumGroupSize)
                    return true;

            }
        }
        return false;
    }

    // Defer so delegates see an initialized grid
    Component.onCompleted: Qt.callLater(reset)

    Timer {
        id: removeTimer

        interval: 220 // pop preview duration
        repeat: false
        onTriggered: {
            console.log("[GameBoard] removeTimer fired");
            doRemoveAndCollapse(pendingRemoval);
            pendingRemoval = ({
            });
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
