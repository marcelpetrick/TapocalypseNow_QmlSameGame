#pragma once

#include <QVector>

struct BoardPosition {
    int row = -1;
    int col = -1;
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

class GameRules {
   public:
    static constexpr int kEmptyCell = -1;

    static GroupAnalysis analyzeSelection(const GameGrid& grid, int row, int col, int minimumGroupSize);
    static MoveResult applyMove(const GameGrid& grid, int row, int col, int minimumGroupSize);
    static GameGrid collapse(const GameGrid& grid);
    static bool hasMoves(const GameGrid& grid, int minimumGroupSize);

   private:
    static bool isInBounds(const GameGrid& grid, int row, int col);
    static QVector<BoardPosition> collectGroup(const GameGrid& grid, int row, int col);
};
