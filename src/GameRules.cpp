#include "GameRules.h"

#include <QQueue>

namespace {

bool isRectangular(const GameGrid& grid) {
    if (grid.isEmpty()) return true;

    const int cols = grid.first().size();
    for (const auto& row : grid) {
        if (row.size() != cols) return false;
    }
    return true;
}  // GCOVR_EXCL_LINE

}  // namespace

bool GameRules::isInBounds(const GameGrid& grid, int row, int col) {
    if (grid.isEmpty()) return false;
    if (row < 0 || row >= grid.size()) return false;
    if (col < 0 || col >= grid.first().size()) return false;
    return true;
}  // GCOVR_EXCL_LINE

QVector<BoardPosition> GameRules::collectGroup(const GameGrid& grid, int row, int col) {
    QVector<BoardPosition> group;
    if (!isRectangular(grid) || !isInBounds(grid, row, col)) return group;

    const int color = grid[row][col];
    if (color == kEmptyCell) return group;

    QVector<QVector<bool>> visited(grid.size(), QVector<bool>(grid.first().size(), false));
    QQueue<BoardPosition> queue;
    queue.enqueue({row, col});
    visited[row][col] = true;

    while (!queue.isEmpty()) {
        const auto current = queue.dequeue();
        if (grid[current.row][current.col] != color) continue;  // GCOVR_EXCL_BR_LINE

        group.push_back(current);

        static constexpr int kDirections[4][2] = {
            {-1, 0},
            {1, 0},
            {0, -1},
            {0, 1},
        };

        for (const auto& direction : kDirections) {
            const int nextRow = current.row + direction[0];
            const int nextCol = current.col + direction[1];
            if (!isInBounds(grid, nextRow, nextCol)) continue;
            if (visited[nextRow][nextCol]) continue;
            visited[nextRow][nextCol] = true;
            if (grid[nextRow][nextCol] == color) queue.enqueue({nextRow, nextCol});
        }
    }

    return group;
}  // GCOVR_EXCL_LINE

GroupAnalysis GameRules::analyzeSelection(const GameGrid& grid, int row, int col, int minimumGroupSize) {
    GroupAnalysis result;
    result.group = collectGroup(grid, row, col);
    result.groupSize = result.group.size();
    result.valid = result.groupSize >= minimumGroupSize;
    return result;  // GCOVR_EXCL_LINE
}  // GCOVR_EXCL_LINE

GroupAnalysis GameRules::findFirstValidSelection(const GameGrid& grid, int minimumGroupSize) {
    GroupAnalysis result;
    if (!isRectangular(grid) || grid.isEmpty()) return result;

    QVector<QVector<bool>> visited(grid.size(), QVector<bool>(grid.first().size(), false));

    for (int row = 0; row < grid.size(); ++row) {
        for (int col = 0; col < grid.first().size(); ++col) {
            if (visited[row][col] || grid[row][col] == kEmptyCell) continue;

            const auto group = collectGroup(grid, row, col);
            for (const auto& position : group) {
                visited[position.row][position.col] = true;
            }

            if (group.size() < minimumGroupSize) continue;

            result.valid = true;
            result.groupSize = group.size();
            result.group = group;
            return result;
        }
    }

    return result;
}  // GCOVR_EXCL_LINE

GameGrid GameRules::removeGroup(const GameGrid& grid, const QVector<BoardPosition>& group) {
    GameGrid nextGrid = grid;
    for (const auto& position : group) {
        if (isInBounds(nextGrid, position.row, position.col)) {
            nextGrid[position.row][position.col] = kEmptyCell;
        }
    }
    return nextGrid;  // GCOVR_EXCL_LINE
}  // GCOVR_EXCL_LINE

GameGrid GameRules::collapseDown(const GameGrid& grid) {
    if (!isRectangular(grid) || grid.isEmpty()) return grid;

    const int rows = grid.size();
    const int cols = grid.first().size();
    GameGrid collapsed(rows, QVector<int>(cols, kEmptyCell));

    for (int col = 0; col < cols; ++col) {
        QVector<int> compactedColumn;
        for (int row = rows - 1; row >= 0; --row) {
            if (grid[row][col] != kEmptyCell) compactedColumn.push_back(grid[row][col]);
        }

        for (int index = 0; index < compactedColumn.size(); ++index) {
            collapsed[rows - 1 - index][col] = compactedColumn[index];
        }
    }

    return collapsed;
}  // GCOVR_EXCL_LINE

GameGrid GameRules::shiftColumnsLeft(const GameGrid& grid) {
    if (!isRectangular(grid) || grid.isEmpty()) return grid;

    const int rows = grid.size();
    const int cols = grid.first().size();
    GameGrid shifted(rows, QVector<int>(cols, kEmptyCell));
    int targetCol = 0;

    for (int sourceCol = 0; sourceCol < cols; ++sourceCol) {
        bool hasTile = false;
        for (int row = 0; row < rows; ++row) {
            if (grid[row][sourceCol] != kEmptyCell) {
                hasTile = true;
                break;
            }
        }

        if (!hasTile) continue;

        for (int row = 0; row < rows; ++row) {
            shifted[row][targetCol] = grid[row][sourceCol];
        }
        ++targetCol;
    }

    return shifted;
}  // GCOVR_EXCL_LINE

GameGrid GameRules::collapse(const GameGrid& grid) { return shiftColumnsLeft(collapseDown(grid)); }

QVector<TileMotion> GameRules::buildFallMotions(const GameGrid& fromGrid, const GameGrid& toGrid) {
    QVector<TileMotion> motions;
    if (!isRectangular(fromGrid) || fromGrid.isEmpty()) return motions;

    const int rows = fromGrid.size();
    const int cols = fromGrid.first().size();

    for (int col = 0; col < cols; ++col) {
        QVector<BoardPosition> sourcePositions;
        QVector<BoardPosition> targetPositions;

        for (int row = rows - 1; row >= 0; --row) {
            if (fromGrid[row][col] != kEmptyCell) sourcePositions.push_back({row, col});
            if (toGrid[row][col] != kEmptyCell) targetPositions.push_back({row, col});
        }

        const int count = qMin(sourcePositions.size(), targetPositions.size());
        for (int index = 0; index < count; ++index) {
            const auto& from = sourcePositions[index];
            const auto& to = targetPositions[index];
            if (from.row == to.row) continue;

            motions.push_back({
                fromGrid[from.row][from.col],
                from,
                to,
            });
        }
    }

    return motions;  // GCOVR_EXCL_LINE
}  // GCOVR_EXCL_LINE

QVector<TileMotion> GameRules::buildShiftMotions(const GameGrid& fromGrid, const GameGrid& toGrid) {
    QVector<TileMotion> motions;
    if (!isRectangular(fromGrid) || fromGrid.isEmpty()) return motions;

    const int rows = fromGrid.size();
    const int cols = fromGrid.first().size();
    int targetCol = 0;

    for (int sourceCol = 0; sourceCol < cols; ++sourceCol) {
        bool hasTile = false;
        for (int row = 0; row < rows; ++row) {
            if (fromGrid[row][sourceCol] != kEmptyCell) {
                hasTile = true;
                break;
            }
        }

        if (!hasTile) continue;

        if (sourceCol != targetCol) {
            for (int row = 0; row < rows; ++row) {
                if (fromGrid[row][sourceCol] == kEmptyCell) continue;
                motions.push_back({
                    fromGrid[row][sourceCol],
                    {row, sourceCol},
                    {row, targetCol},
                });
            }
        }

        ++targetCol;
    }

    Q_UNUSED(toGrid);
    return motions;  // GCOVR_EXCL_LINE
}  // GCOVR_EXCL_LINE

bool GameRules::hasMoves(const GameGrid& grid, int minimumGroupSize) { return findFirstValidSelection(grid, minimumGroupSize).valid; }

MoveResult GameRules::applyMove(const GameGrid& grid, int row, int col, int minimumGroupSize) {
    MoveResult result;
    result.grid = grid;

    const auto analysis = analyzeSelection(grid, row, col, minimumGroupSize);
    result.group = analysis.group;
    if (!analysis.valid) {
        result.hasMoves = hasMoves(grid, minimumGroupSize);
        return result;
    }

    const GameGrid nextGrid = removeGroup(grid, analysis.group);

    result.valid = true;
    result.removedCount = analysis.groupSize;
    result.grid = collapse(nextGrid);
    result.hasMoves = hasMoves(result.grid, minimumGroupSize);
    return result;
}

MovePlan GameRules::planMove(const GameGrid& grid, int row, int col, int minimumGroupSize) {
    MovePlan result;

    const auto analysis = analyzeSelection(grid, row, col, minimumGroupSize);
    result.group = analysis.group;
    if (!analysis.valid) {
        result.afterRemovalGrid = grid;
        result.afterFallGrid = grid;
        result.afterShiftGrid = grid;
        result.hasMoves = hasMoves(grid, minimumGroupSize);
        return result;
    }

    result.valid = true;
    result.removedCount = analysis.groupSize;
    result.afterRemovalGrid = removeGroup(grid, analysis.group);
    result.afterFallGrid = collapseDown(result.afterRemovalGrid);
    result.afterShiftGrid = shiftColumnsLeft(result.afterFallGrid);
    result.fallMotions = buildFallMotions(result.afterRemovalGrid, result.afterFallGrid);
    result.shiftMotions = buildShiftMotions(result.afterFallGrid, result.afterShiftGrid);
    result.hasMoves = hasMoves(result.afterShiftGrid, minimumGroupSize);
    return result;
}
