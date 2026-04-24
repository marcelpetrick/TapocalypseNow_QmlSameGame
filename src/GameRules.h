#pragma once

#include <QVector>

/**
 * @brief Identifies a tile location inside a board.
 */
struct BoardPosition {
    /// Zero-based row index.
    int row = -1;
    /// Zero-based column index.
    int col = -1;
};

/**
 * @brief Describes one animated tile movement between two board states.
 */
struct TileMotion {
    /// Tile color identifier carried by the moving tile.
    int color = -1;
    /// Source position before the move.
    BoardPosition from;
    /// Destination position after the move.
    BoardPosition to;
};

/// Two-dimensional board storage using color ids and @ref GameRules::kEmptyCell markers.
using GameGrid = QVector<QVector<int>>;

/**
 * @brief Result of selecting one tile on the board.
 */
struct GroupAnalysis {
    /// True when the connected group meets the requested minimum size.
    bool valid = false;
    /// Number of connected tiles found at the queried position.
    int groupSize = 0;
    /// Positions that belong to the connected group.
    QVector<BoardPosition> group;
};

/**
 * @brief Result of applying one complete move to a board.
 */
struct MoveResult {
    /// True when the requested move removed a valid group.
    bool valid = false;
    /// Number of tiles removed by the move.
    int removedCount = 0;
    /// True when the resulting board still has a playable group.
    bool hasMoves = false;
    /// Board state after removal, gravity, and column shifting.
    GameGrid grid;
    /// Positions that were selected for the move.
    QVector<BoardPosition> group;
};

/**
 * @brief Staged representation of a move for animation and UI orchestration.
 */
struct MovePlan {
    /// True when the requested move removed a valid group.
    bool valid = false;
    /// Number of tiles removed by the move.
    int removedCount = 0;
    /// True when the fully resolved board still has a playable group.
    bool hasMoves = false;
    /// Board immediately after removing the selected group.
    GameGrid afterRemovalGrid;
    /// Board after tiles have fallen vertically.
    GameGrid afterFallGrid;
    /// Final board after empty columns have shifted left.
    GameGrid afterShiftGrid;
    /// Positions that were selected for the move.
    QVector<BoardPosition> group;
    /// Tile movements required for the fall phase.
    QVector<TileMotion> fallMotions;
    /// Tile movements required for the shift phase.
    QVector<TileMotion> shiftMotions;
};

/**
 * @brief Pure board-manipulation rules for Tapocalypse.
 *
 * This class contains no UI dependencies and is intended to stay unit-testable.
 */
class GameRules {
   public:
    /// Sentinel value used for empty cells in a board.
    static constexpr int kEmptyCell = -1;

    /**
     * @brief Analyze the connected group at one board position.
     * @param grid Board to inspect.
     * @param row Zero-based row index.
     * @param col Zero-based column index.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Group membership, size, and validity for the queried position.
     */
    static GroupAnalysis analyzeSelection(const GameGrid& grid, int row, int col, int minimumGroupSize);
    /**
     * @brief Find the first playable group in scan order.
     * @param grid Board to inspect.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return First valid selectable group, or an invalid result when none exists.
     */
    static GroupAnalysis findFirstValidSelection(const GameGrid& grid, int minimumGroupSize);
    /**
     * @brief Apply one move and fully resolve the resulting board.
     * @param grid Board to mutate logically.
     * @param row Zero-based row index.
     * @param col Zero-based column index.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Final move result after removal and collapse.
     */
    static MoveResult applyMove(const GameGrid& grid, int row, int col, int minimumGroupSize);
    /**
     * @brief Build the staged intermediate states for one move.
     * @param grid Board to inspect.
     * @param row Zero-based row index.
     * @param col Zero-based column index.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return Intermediate grids and tile motions needed to animate the move.
     */
    static MovePlan planMove(const GameGrid& grid, int row, int col, int minimumGroupSize);
    /**
     * @brief Replace all positions in a selected group with empty cells.
     * @param grid Source board.
     * @param group Positions to clear.
     * @return Board after clearing the selected positions.
     */
    static GameGrid removeGroup(const GameGrid& grid, const QVector<BoardPosition>& group);
    /**
     * @brief Apply gravity inside each column.
     * @param grid Source board.
     * @return Board after non-empty cells fall downward within their columns.
     */
    static GameGrid collapseDown(const GameGrid& grid);
    /**
     * @brief Shift non-empty columns left while preserving their order.
     * @param grid Source board.
     * @return Board after empty columns have been removed from the left-to-right layout.
     */
    static GameGrid shiftColumnsLeft(const GameGrid& grid);
    /**
     * @brief Fully collapse a board by applying gravity and then shifting columns.
     * @param grid Source board.
     * @return Fully resolved board state.
     */
    static GameGrid collapse(const GameGrid& grid);
    /**
     * @brief Check whether any valid move is available.
     * @param grid Board to inspect.
     * @param minimumGroupSize Minimum group size required for a valid move.
     * @return True when at least one selectable group exists.
     */
    static bool hasMoves(const GameGrid& grid, int minimumGroupSize);

   private:
    static bool isInBounds(const GameGrid& grid, int row, int col);
    static QVector<BoardPosition> collectGroup(const GameGrid& grid, int row, int col);
    static QVector<TileMotion> buildFallMotions(const GameGrid& fromGrid, const GameGrid& toGrid);
    static QVector<TileMotion> buildShiftMotions(const GameGrid& fromGrid, const GameGrid& toGrid);
};
