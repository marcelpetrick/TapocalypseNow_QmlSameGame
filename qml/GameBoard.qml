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

    function reset() {
        score = 0; moves = 0; gameOver = false
        grid = []
        for (let r = 0; r < rows; ++r) {
            let row = []
            for (let c = 0; c < cols; ++c) {
                row.push(Math.floor(Math.random() * neonPalette().length))
            }
            grid.push(row)
        }
        repeater.model = rows * cols
    }

    function inBounds(r, c) { return r >= 0 && r < rows && c >= 0 && c < cols; }

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
        if (size < 2) return

        for (let key in visited) {
            const [rr, cc] = key.split(",").map(Number)
            grid[rr][cc] = -1
        }

        score += size * size
        moves += 1
        collapse()
        if (!hasMoves()) {
            gameOver = true
            gameEnded(score)
        }
    }

    function collapse() {
        // drop down
        for (let c = 0; c < cols; ++c) {
            let write = rows - 1
            for (let r = rows - 1; r >= 0; --r) {
                if (grid[r][c] !== -1) {
                    if (write !== r) grid[write][c] = grid[r][c]
                    write--
                }
            }
            for (let r = write; r >= 0; --r) grid[r][c] = -1
        }
        // shift left
        let writeC = 0
        for (let c = 0; c < cols; ++c) {
            let has = false
            for (let r = 0; r < rows; ++r) if (grid[r][c] !== -1) { has = true; break }
            if (has) {
                if (writeC !== c) {
                    for (let r = 0; r < rows; ++r) { grid[r][writeC] = grid[r][c]; grid[r][c] = -1 }
                }
                writeC++
            }
        }
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

    // renamed to avoid clashing with the auto-generated 'gridChanged' signal
    function refreshGrid() { tileView.forceLayout() }

    Component.onCompleted: reset()

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
                colorIndex: board.grid[r][c]
                palette: board.neonPalette()
                onClicked: board.removeGroup(r, c)
            }
        }
    }
}
