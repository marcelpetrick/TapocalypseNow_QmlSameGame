#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief Variant-based bridge that exposes pure game rules to QML and tests.
 *
 * The grid wire format is a nested @c QVariantList of rows containing integer cell values. Empty cells use
 * @ref GameRules::kEmptyCell. Selection maps use keys in the form @c "row,col" with boolean @c true values. This
 * class performs data-shape translation only and delegates gameplay semantics to @ref GameRules.
 */
class GameLogicBridge : public QObject {
    Q_OBJECT

   public:
    /**
     * @brief Create the bridge.
     * @param parent Owning QObject parent.
     */
    explicit GameLogicBridge(QObject* parent = nullptr);

    /**
     * @brief Analyze the connected group at one position in a variant-backed grid.
     * @param rawGrid Grid encoded as nested QVariantList rows.
     * @param row Zero-based row index.
     * @param col Zero-based column index.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Map with keys @c valid, @c groupSize, and @c selection. The @c selection value is a lookup map using
     * @c "row,col" keys. Invalid input returns @c valid == false and an empty selection.
     */
    Q_INVOKABLE QVariantMap analyzeSelection(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const;
    /**
     * @brief Find the first playable group in scan order.
     * @param rawGrid Grid encoded as nested QVariantList rows.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Map with keys @c valid, @c groupSize, and @c selection. The search order is top-to-bottom,
     * left-to-right. When no valid group exists, @c valid is false and @c selection is empty.
     */
    Q_INVOKABLE QVariantMap findFirstValidSelection(const QVariantList& rawGrid, int minimumGroupSize) const;
    /**
     * @brief Apply one move and return the fully collapsed grid.
     * @param rawGrid Grid encoded as nested QVariantList rows.
     * @param row Zero-based row index.
     * @param col Zero-based column index.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Map with keys @c valid, @c removedCount, @c hasMoves, @c selection, and @c grid. Invalid moves keep the
     * original grid in @c grid and report @c valid == false.
     */
    Q_INVOKABLE QVariantMap applyMove(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const;
    /**
     * @brief Build the staged intermediate states for animating one move.
     * @param rawGrid Grid encoded as nested QVariantList rows.
     * @param row Zero-based row index.
     * @param col Zero-based column index.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Map with keys @c valid, @c removedCount, @c hasMoves, @c selection, @c afterRemovalGrid,
     * @c afterFallGrid, @c afterShiftGrid, @c fallMotions, and @c shiftMotions. Motion items contain @c color,
     * @c fromRow, @c fromCol, @c toRow, and @c toCol. Invalid moves preserve the input grid in every staged output and
     * return empty motion lists.
     */
    Q_INVOKABLE QVariantMap planMove(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const;
    /**
     * @brief Check whether the supplied board still has a valid move.
     * @param rawGrid Grid encoded as nested QVariantList rows.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return True when at least one selectable group exists. Invalid or irregular grids return false.
     */
    Q_INVOKABLE bool hasMoves(const QVariantList& rawGrid, int minimumGroupSize) const;
};
