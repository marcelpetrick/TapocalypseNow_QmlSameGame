#include "GameLogicBridge.h"

#include "GameRules.h"
#include "VariantConversions.h"

using namespace VariantConversions;

GameLogicBridge::GameLogicBridge(QObject* parent) : QObject(parent) {}

QVariantMap GameLogicBridge::analyzeSelection(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const {
    const auto analysis = GameRules::analyzeSelection(toGameGrid(rawGrid), row, col, minimumGroupSize);

    QVariantMap result;
    result.insert("valid", analysis.valid);
    result.insert("groupSize", analysis.groupSize);
    result.insert("selection", toSelectionMap(analysis.group));
    return result;
}

QVariantMap GameLogicBridge::findFirstValidSelection(const QVariantList& rawGrid, int minimumGroupSize) const {
    const auto analysis = GameRules::findFirstValidSelection(toGameGrid(rawGrid), minimumGroupSize);

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
