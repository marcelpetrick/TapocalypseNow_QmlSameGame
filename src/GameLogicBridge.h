#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

/**
 * @brief Variant-based bridge that exposes pure game rules to QML and tests.
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
     * @return Map containing validity, group size, and a selection lookup map.
     */
    Q_INVOKABLE QVariantMap analyzeSelection(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const;
    /**
     * @brief Find the first playable group in scan order.
     * @param rawGrid Grid encoded as nested QVariantList rows.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Map containing validity, group size, and a selection lookup map.
     */
    Q_INVOKABLE QVariantMap findFirstValidSelection(const QVariantList& rawGrid, int minimumGroupSize) const;
    /**
     * @brief Apply one move and return the fully collapsed grid.
     * @param rawGrid Grid encoded as nested QVariantList rows.
     * @param row Zero-based row index.
     * @param col Zero-based column index.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Map containing move validity, removed count, remaining-move state, selection, and final grid.
     */
    Q_INVOKABLE QVariantMap applyMove(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const;
    /**
     * @brief Build the staged intermediate states for animating one move.
     * @param rawGrid Grid encoded as nested QVariantList rows.
     * @param row Zero-based row index.
     * @param col Zero-based column index.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Map containing staged grids and tile motions for the move.
     */
    Q_INVOKABLE QVariantMap planMove(const QVariantList& rawGrid, int row, int col, int minimumGroupSize) const;
    /**
     * @brief Check whether the supplied board still has a valid move.
     * @param rawGrid Grid encoded as nested QVariantList rows.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return True when at least one selectable group exists.
     */
    Q_INVOKABLE bool hasMoves(const QVariantList& rawGrid, int minimumGroupSize) const;
};
