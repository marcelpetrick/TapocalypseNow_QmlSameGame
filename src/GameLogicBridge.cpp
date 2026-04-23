#include "GameLogicBridge.h"

#include "GameRules.h"

namespace {

GameGrid toGameGrid(const QVariantList& rawGrid) {
    GameGrid grid;
    grid.reserve(rawGrid.size());

    for (const auto& rowValue : rawGrid) {
        const auto rawRow = rowValue.toList();
        QVector<int> row;
        row.reserve(rawRow.size());
        for (const auto& cellValue : rawRow) {
            row.push_back(cellValue.toInt());
        }
        grid.push_back(row);
    }

    return grid;
}

QVariantList toVariantGrid(const GameGrid& grid) {
    QVariantList rawGrid;
    rawGrid.reserve(grid.size());

    for (const auto& row : grid) {
        QVariantList rawRow;
        rawRow.reserve(row.size());
        for (const auto cell : row) {
            rawRow.push_back(cell);
        }
        rawGrid.push_back(rawRow);
    }

    return rawGrid;
}

QVariantMap toSelectionMap(const QVector<BoardPosition>& group) {
    QVariantMap selection;
    for (const auto& position : group) {
        selection.insert(QStringLiteral("%1,%2").arg(position.row).arg(position.col), true);
    }
    return selection;
}

}  // namespace

GameLogicBridge::GameLogicBridge(QObject* parent) : QObject(parent) {}

QVariantMap GameLogicBridge::analyzeSelection(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const {
    const auto analysis = GameRules::analyzeSelection(toGameGrid(rawGrid), row, col, minimumGroupSize);

    QVariantMap result;
    result.insert("valid", analysis.valid);
    result.insert("groupSize", analysis.groupSize);
    result.insert("selection", toSelectionMap(analysis.group));
    return result;
}

QVariantMap GameLogicBridge::applyMove(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const {
    const auto move = GameRules::applyMove(toGameGrid(rawGrid), row, col, minimumGroupSize);

    QVariantMap result;
    result.insert("valid", move.valid);
    result.insert("removedCount", move.removedCount);
    result.insert("hasMoves", move.hasMoves);
    result.insert("selection", toSelectionMap(move.group));
    result.insert("grid", toVariantGrid(move.grid));
    return result;
}

bool GameLogicBridge::hasMoves(const QVariantList& rawGrid, int minimumGroupSize) const { return GameRules::hasMoves(toGameGrid(rawGrid), minimumGroupSize); }
