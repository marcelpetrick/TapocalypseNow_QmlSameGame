/**
 * Rule conformance: plays thousands of seeded random games and checks every move against RULES.md
 * using an independent, deliberately simple reference implementation.
 */

#include <QRandomGenerator>
#include <QtTest>

#include "GameRules.h"

namespace {

constexpr int kMinimumGroupSize = 3;  // RULES.md §3

GameGrid randomGrid(QRandomGenerator& rng, int rows, int cols, int colors) {
    GameGrid grid(rows, QVector<int>(cols));
    for (auto& row : grid) {
        for (auto& cell : row) cell = rng.bounded(colors);
    }
    return grid;
}

// Reference flood fill: repeat "add every same-coloured orthogonal neighbour" until nothing changes.
QSet<QPair<int, int>> referenceGroup(const GameGrid& grid, int row, int col) {
    QSet<QPair<int, int>> group;
    const int color = grid[row][col];
    if (color == GameRules::kEmptyCell) return group;
    group.insert({row, col});
    bool grew = true;
    while (grew) {
        grew = false;
        for (const auto& [r, c] : QSet<QPair<int, int>>(group)) {
            const QPair<int, int> neighbours[] = {{r - 1, c}, {r + 1, c}, {r, c - 1}, {r, c + 1}};
            for (const auto& [nr, nc] : neighbours) {
                if (nr < 0 || nc < 0 || nr >= grid.size() || nc >= grid[0].size()) continue;
                if (grid[nr][nc] == color && !group.contains({nr, nc})) {
                    group.insert({nr, nc});
                    grew = true;
                }
            }
        }
    }
    return group;
}

// Column contents from bottom to top, ignoring empty cells.
QVector<int> stack(const GameGrid& grid, int col) {
    QVector<int> cells;
    for (int row = grid.size() - 1; row >= 0; --row) {
        if (grid[row][col] != GameRules::kEmptyCell) cells.push_back(grid[row][col]);
    }
    return cells;
}

int tileCount(const GameGrid& grid) {
    int count = 0;
    for (const auto& row : grid) count += static_cast<int>(std::count_if(row.cbegin(), row.cend(), [](int cell) { return cell != GameRules::kEmptyCell; }));
    return count;
}

bool referenceHasMoves(const GameGrid& grid) {
    for (int row = 0; row < grid.size(); ++row) {
        for (int col = 0; col < grid[0].size(); ++col) {
            if (referenceGroup(grid, row, col).size() >= kMinimumGroupSize) return true;
        }
    }
    return false;
}

}  // namespace

class RulesConformanceTest : public QObject {
    Q_OBJECT

   private slots:
    void randomGames_obeyRules_data();
    void randomGames_obeyRules();
};

void RulesConformanceTest::randomGames_obeyRules_data() {
    QTest::addColumn<int>("rows");
    QTest::addColumn<int>("cols");
    QTest::addColumn<int>("colors");
    QTest::addColumn<int>("games");
    QTest::newRow("shipped board 10x16, 6 colours") << 10 << 16 << 6 << 15;
    QTest::newRow("dense 8x8, 3 colours") << 8 << 8 << 3 << 40;
    QTest::newRow("tall 12x4, 4 colours") << 12 << 4 << 4 << 40;
    QTest::newRow("single row 1x20, 2 colours") << 1 << 20 << 2 << 100;
    QTest::newRow("single column 20x1, 2 colours") << 20 << 1 << 2 << 100;
}

void RulesConformanceTest::randomGames_obeyRules() {
    QFETCH(int, rows);
    QFETCH(int, cols);
    QFETCH(int, colors);
    QFETCH(int, games);

    QRandomGenerator rng(0x7A90CA1u + static_cast<quint32>(rows * 131 + cols * 17 + colors));
    int movesChecked = 0;

    for (int game = 0; game < games; ++game) {
        GameGrid grid = randomGrid(rng, rows, cols, colors);
        int score = 0;

        while (true) {
            // §10: the game is over exactly when no group of three or more exists.
            QCOMPARE(GameRules::hasMoves(grid, kMinimumGroupSize), referenceHasMoves(grid));
            if (!referenceHasMoves(grid)) break;

            // §5: every tile reports its full group, and the group is legal exactly when it has 3+ tiles.
            QVector<BoardPosition> legalTiles;
            for (int row = 0; row < rows; ++row) {
                for (int col = 0; col < cols; ++col) {
                    const auto analysis = GameRules::analyzeSelection(grid, row, col, kMinimumGroupSize);
                    const auto expected = referenceGroup(grid, row, col);
                    QCOMPARE(analysis.groupSize, static_cast<int>(expected.size()));
                    QCOMPARE(analysis.valid, expected.size() >= kMinimumGroupSize);
                    for (const auto& position : analysis.group) QVERIFY(expected.contains({position.row, position.col}));
                    if (analysis.valid) legalTiles.push_back({row, col});
                    if (!analysis.valid) {
                        // Illegal clicks change nothing.
                        const auto move = GameRules::applyMove(grid, row, col, kMinimumGroupSize);
                        QVERIFY(!move.valid);
                        QCOMPARE(move.grid, grid);
                    }
                }
            }

            const auto pick = legalTiles[rng.bounded(static_cast<int>(legalTiles.size()))];
            const auto removed = referenceGroup(grid, pick.row, pick.col);
            const auto move = GameRules::applyMove(grid, pick.row, pick.col, kMinimumGroupSize);
            const auto plan = GameRules::planMove(grid, pick.row, pick.col, kMinimumGroupSize);

            // §6/§7: the whole group goes and scores N*N.
            QVERIFY(move.valid);
            QCOMPARE(move.removedCount, static_cast<int>(removed.size()));
            QCOMPARE(tileCount(move.grid), tileCount(grid) - move.removedCount);
            score += move.removedCount * move.removedCount;

            // §8: column stacks keep their order minus the removed tiles; non-empty columns keep their order.
            QVector<QVector<int>> expectedColumns;
            for (int col = 0; col < cols; ++col) {
                QVector<int> kept;
                for (int row = rows - 1; row >= 0; --row) {
                    if (grid[row][col] != GameRules::kEmptyCell && !removed.contains({row, col})) kept.push_back(grid[row][col]);
                }
                if (!kept.isEmpty()) expectedColumns.push_back(kept);
            }
            for (int col = 0; col < cols; ++col) {
                const auto actual = stack(move.grid, col);
                QCOMPARE(actual, col < expectedColumns.size() ? expectedColumns[col] : QVector<int>{});
                // §13: no tile floats above an empty cell.
                for (int row = 0; row < rows - static_cast<int>(actual.size()); ++row) QCOMPARE(move.grid[row][col], GameRules::kEmptyCell);
            }

            // The animated staging must land on the same board as the direct resolution.
            QCOMPARE(plan.afterShiftGrid, move.grid);
            QCOMPARE(plan.hasMoves, move.hasMoves);
            GameGrid replayed = plan.afterRemovalGrid;
            for (const auto& motion : plan.fallMotions) replayed[motion.from.row][motion.from.col] = GameRules::kEmptyCell;
            for (const auto& motion : plan.fallMotions) replayed[motion.to.row][motion.to.col] = motion.color;
            QCOMPARE(replayed, plan.afterFallGrid);

            grid = move.grid;
            ++movesChecked;
        }

        QVERIFY(score >= 0);
    }

    QVERIFY(movesChecked > games);
}

QTEST_APPLESS_MAIN(RulesConformanceTest)

#include "test_rules_conformance.moc"
