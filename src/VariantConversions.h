#pragma once

#include <QVariantList>
#include <QVariantMap>
#include <optional>

#include "GameRules.h"

/**
 * @brief Conversions between native board types and the QVariant wire format shared with QML.
 *
 * Grids are nested row-major @c QVariantList values of integers, selections are maps keyed by @c "row,col" with
 * @c true values, and motions are maps with @c color, @c fromRow, @c fromCol, @c toRow, @c toCol and an optional
 * @c duration.
 */
namespace VariantConversions {

/// @return Lookup key used by selection maps for one cell.
inline QString positionKey(int row, int col) { return QStringLiteral("%1,%2").arg(row).arg(col); }

/// @return Native grid decoded from nested variant rows; non-numeric cells become @c 0.
inline GameGrid toGameGrid(const QVariantList& rawGrid) {
    GameGrid grid;
    grid.reserve(rawGrid.size());
    for (const auto& rowValue : rawGrid) {
        const auto rawRow = rowValue.toList();
        QVector<int> row;
        row.reserve(rawRow.size());
        for (const auto& cellValue : rawRow) row.push_back(cellValue.toInt());
        grid.push_back(row);
    }
    return grid;
}

/// @return Grid encoded as nested variant rows.
inline QVariantList toVariantGrid(const GameGrid& grid) {
    QVariantList rawGrid;
    rawGrid.reserve(grid.size());
    for (const auto& row : grid) {
        QVariantList rawRow;
        rawRow.reserve(row.size());
        for (const auto cell : row) rawRow.push_back(cell);
        rawGrid.push_back(rawRow);
    }
    return rawGrid;
}

/// @return Selection map containing one @c true entry per position.
inline QVariantMap toSelectionMap(const QVector<BoardPosition>& group) {
    QVariantMap selection;
    for (const auto& position : group) selection.insert(positionKey(position.row, position.col), true);
    return selection;
}

/// @return Motion maps; @c duration is only included when @p durationMs is given.
inline QVariantList toMotions(const QVector<TileMotion>& motions, std::optional<int> durationMs = std::nullopt) {
    QVariantList rawMotions;
    rawMotions.reserve(motions.size());
    for (const auto& motion : motions) {
        QVariantMap item;
        item.insert("color", motion.color);
        item.insert("fromRow", motion.from.row);
        item.insert("fromCol", motion.from.col);
        item.insert("toRow", motion.to.row);
        item.insert("toCol", motion.to.col);
        if (durationMs) item.insert("duration", *durationMs);
        rawMotions.push_back(item);
    }
    return rawMotions;
}

}  // namespace VariantConversions
