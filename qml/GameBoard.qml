import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: board
    property int rows: 10
    property int cols: 16
    readonly property real cell: Math.min(width / cols, height / rows)
    property int score: 0
    property int moves: 0
    property bool gameOver: false

    signal gameEnded(int score)

    // Internal model: -1 empty; >=0 color index
    property var grid: []

    function neonPalette() {
        return ["#00E5FF", "#FF00E5", "#00FFAE", "#FFD400", "#8B80FF", "#FF5E5B"];
    }

    // ---- helpers -----------------------------------------------------------
    function inBounds(r, c) { return r >= 0 && r < rows && c >= 0 && c < cols; }

    function rebuildDelegates() {
        const m = rows * cols
        if (repeater.model === m) {
            repeater.model = 0
        }
        repeater.model = m
    }

    function refreshGrid() { tileView.forceLayout() }

    // ---- game logic --------------------------------------------------------
    function reset() {
        console.log("[GameBoard] reset()")

        score = 0; moves = 0; gameOver = false

        // Build a NEW grid and assign once (triggers change notification)
        const paletteLen = neonPalette().length
        let newGrid = []
        for (let r = 0; r < rows; ++r) {
            let row = new Array(cols)
            for (let c = 0; c < cols; ++c)
                row[c] = Math.floor(Math.random() * paletteLen)
            newGrid.push(row)
        }
        grid = newGrid
        console.log("[GameBoard] reset() -> grid assigned, size:", rows, "x", cols)

        rebuildDelegates()
    }

    function flood(r, c, color, visited) {
        const key = r + "," + c
        if (visited[key]) return 0
        visited[key] = true
        if (!inBounds(r, c)) return 0
        if (grid[r][c] !== color) return 0
        let count = 1
        count += flood(r + 1, c, color, visited)
        count += flood(r - 1, c, color, visited)
        count += flood(r, c + 1, color, visited)
        count += flood(r, c - 1, color, visited)
        return count
    }

    function removeGroup(r, c) {
        if (!inBounds(r, c)) return
        const color = grid[r][c]
        if (color === -1) return

        const visited = {}
        const size = flood(r, c, color, visited)
        console.log("[GameBoard] removeGroup(", r, c, ") color:", color, "size:", size)
        if (size < 2) return

        // Work on a copy, then reassign
        let g = grid.map(row => row.slice())
        for (let key in visited) {
            const [rr, cc] = key.split(",").map(Number)
            g[rr][cc] = -1
        }
        grid = g

        score += size * size
        moves += 1
        console.log("[GameBoard] removeGroup -> score:", score, "moves:", moves)

        collapse()

        if (!hasMoves()) {
            console.log("[GameBoard] no more moves -> game over, score:", score)
            gameOver = true
            gameEnded(score)
        }
    }

    function collapse() {
        console.log("[GameBoard] collapse()")
        // Copy, mutate, then reassign
        let g = grid.map(row => row.slice())

        // drop down
        for (let c = 0; c < cols; ++c) {
            let write = rows - 1
            for (let r = rows - 1; r >= 0; --r) {
                if (g[r][c] !== -1) {
                    if (write !== r) g[write][c] = g[r][c]
                    write--
                }
            }
            for (let r = write; r >= 0; --r) g[r][c] = -1
        }

        // shift left
        let writeC = 0
        for (let c = 0; c < cols; ++c) {
            let has = false
            for (let r = 0; r < rows; ++r) if (g[r][c] !== -1) { has = true; break }
            if (has) {
                if (writeC !== c) {
                    for (let r = 0; r < rows; ++r) { g[r][writeC] = g[r][c]; g[r][c] = -1 }
                }
                writeC++
            }
        }

        grid = g
        console.log("[GameBoard] collapse() -> grid reassigned")

        rebuildDelegates()
        refreshGrid()
    }

    function hasMoves() {
        for (let r = 0; r < rows; ++r) {
            for (let c = 0; c < cols; ++c) {
                const col = grid[r][c]
                if (col === -1) continue
                if (inBounds(r + 1, c) && grid[r + 1][c] === col) return true
                if (inBounds(r, c + 1) && grid[r][c + 1] === col) return true
            }
        }
        return false
    }

    // Defer so delegates see an initialized grid (prevents early undefined reads)
    Component.onCompleted: Qt.callLater(reset)

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
                colorIndex: (board.grid[r] !== undefined && board.grid[r][c] !== undefined)
                            ? board.grid[r][c] : -1

                palette: board.neonPalette()
                onClicked: {
                    console.log("[Tile] clicked r:", r, "c:", c, "color:", colorIndex)
                    board.removeGroup(r, c)
                }
            }
        }
    }
}
