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
}

}  // namespace

bool GameRules::isInBounds(const GameGrid& grid, int row, int col) {
    if (grid.isEmpty()) return false;
    if (row < 0 || row >= grid.size()) return false;
    if (col < 0 || col >= grid.first().size()) return false;
    return true;
}

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
        if (grid[current.row][current.col] != color) continue;

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
}

GroupAnalysis GameRules::analyzeSelection(const GameGrid& grid, int row, int col, int minimumGroupSize) {
    GroupAnalysis result;
    result.group = collectGroup(grid, row, col);
    result.groupSize = result.group.size();
    result.valid = result.groupSize >= minimumGroupSize;
    return result;
}

GameGrid GameRules::collapse(const GameGrid& grid) {
    if (!isRectangular(grid) || grid.isEmpty()) return grid;

    const int rows = grid.size();
    const int cols = grid.first().size();
    GameGrid collapsed(rows, QVector<int>(cols, kEmptyCell));
    int targetCol = 0;

    for (int sourceCol = 0; sourceCol < cols; ++sourceCol) {
        QVector<int> compactedColumn;
        for (int row = rows - 1; row >= 0; --row) {
            if (grid[row][sourceCol] != kEmptyCell) compactedColumn.push_back(grid[row][sourceCol]);
        }

        if (compactedColumn.isEmpty()) continue;

        for (int index = 0; index < compactedColumn.size(); ++index) {
            collapsed[rows - 1 - index][targetCol] = compactedColumn[index];
        }
        ++targetCol;
    }

    return collapsed;
}

bool GameRules::hasMoves(const GameGrid& grid, int minimumGroupSize) {
    if (!isRectangular(grid) || grid.isEmpty()) return false;

    QVector<QVector<bool>> visited(grid.size(), QVector<bool>(grid.first().size(), false));

    for (int row = 0; row < grid.size(); ++row) {
        for (int col = 0; col < grid.first().size(); ++col) {
            if (visited[row][col] || grid[row][col] == kEmptyCell) continue;

            const auto group = collectGroup(grid, row, col);
            for (const auto& position : group) {
                visited[position.row][position.col] = true;
            }

            if (group.size() >= minimumGroupSize) return true;
        }
    }

    return false;
}

MoveResult GameRules::applyMove(const GameGrid& grid, int row, int col, int minimumGroupSize) {
    MoveResult result;
    result.grid = grid;

    const auto analysis = analyzeSelection(grid, row, col, minimumGroupSize);
    result.group = analysis.group;
    if (!analysis.valid) {
        result.hasMoves = hasMoves(grid, minimumGroupSize);
        return result;
    }

    GameGrid nextGrid = grid;
    for (const auto& position : analysis.group) {
        nextGrid[position.row][position.col] = kEmptyCell;
    }

    result.valid = true;
    result.removedCount = analysis.groupSize;
    result.grid = collapse(nextGrid);
    result.hasMoves = hasMoves(result.grid, minimumGroupSize);
    return result;
}
