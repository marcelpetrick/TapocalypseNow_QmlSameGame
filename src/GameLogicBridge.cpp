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

QVariantList toMotions(const QVector<TileMotion>& motions) {
    QVariantList rawMotions;
    rawMotions.reserve(motions.size());

    for (const auto& motion : motions) {
        QVariantMap item;
        item.insert("color", motion.color);
        item.insert("fromRow", motion.from.row);
        item.insert("fromCol", motion.from.col);
        item.insert("toRow", motion.to.row);
        item.insert("toCol", motion.to.col);
        rawMotions.push_back(item);
    }

    return rawMotions;
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

QVariantMap GameLogicBridge::planMove(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const {
    const auto move = GameRules::planMove(toGameGrid(rawGrid), row, col, minimumGroupSize);

    QVariantMap result;
    result.insert("valid", move.valid);
    result.insert("removedCount", move.removedCount);
    result.insert("hasMoves", move.hasMoves);
    result.insert("selection", toSelectionMap(move.group));
    result.insert("afterRemovalGrid", toVariantGrid(move.afterRemovalGrid));
    result.insert("afterFallGrid", toVariantGrid(move.afterFallGrid));
    result.insert("afterShiftGrid", toVariantGrid(move.afterShiftGrid));
    result.insert("fallMotions", toMotions(move.fallMotions));
    result.insert("shiftMotions", toMotions(move.shiftMotions));
    return result;
}

bool GameLogicBridge::hasMoves(const QVariantList& rawGrid, int minimumGroupSize) const { return GameRules::hasMoves(toGameGrid(rawGrid), minimumGroupSize); }
