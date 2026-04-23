#pragma once

#include <QVector>

struct BoardPosition {
    int row = -1;
    int col = -1;
};

struct TileMotion {
    int color = -1;
    BoardPosition from;
    BoardPosition to;
};

using GameGrid = QVector<QVector<int>>;

struct GroupAnalysis {
    bool valid = false;
    int groupSize = 0;
    QVector<BoardPosition> group;
};

struct MoveResult {
    bool valid = false;
    int removedCount = 0;
    bool hasMoves = false;
    GameGrid grid;
    QVector<BoardPosition> group;
};

struct MovePlan {
    bool valid = false;
    int removedCount = 0;
    bool hasMoves = false;
    GameGrid afterRemovalGrid;
    GameGrid afterFallGrid;
    GameGrid afterShiftGrid;
    QVector<BoardPosition> group;
    QVector<TileMotion> fallMotions;
    QVector<TileMotion> shiftMotions;
};

class GameRules {
   public:
    static constexpr int kEmptyCell = -1;

    static GroupAnalysis analyzeSelection(const GameGrid& grid, int row, int col, int minimumGroupSize);
    static MoveResult applyMove(const GameGrid& grid, int row, int col, int minimumGroupSize);
    static MovePlan planMove(const GameGrid& grid, int row, int col, int minimumGroupSize);
    static GameGrid removeGroup(const GameGrid& grid, const QVector<BoardPosition>& group);
    static GameGrid collapseDown(const GameGrid& grid);
    static GameGrid shiftColumnsLeft(const GameGrid& grid);
    static GameGrid collapse(const GameGrid& grid);
    static bool hasMoves(const GameGrid& grid, int minimumGroupSize);

   private:
    static bool isInBounds(const GameGrid& grid, int row, int col);
    static QVector<BoardPosition> collectGroup(const GameGrid& grid, int row, int col);
    static QVector<TileMotion> buildFallMotions(const GameGrid& fromGrid, const GameGrid& toGrid);
    static QVector<TileMotion> buildShiftMotions(const GameGrid& fromGrid, const GameGrid& toGrid);
};
