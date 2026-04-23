#include <QtTest>

#include "GameRules.h"

namespace {

GameGrid makeGrid(std::initializer_list<std::initializer_list<int>> rows) {
    GameGrid grid;
    for (const auto& rowValues : rows) {
        QVector<int> row;
        row.reserve(static_cast<int>(rowValues.size()));
        for (const auto value : rowValues) {
            row.push_back(value);
        }
        grid.push_back(row);
    }
    return grid;
}

}  // namespace

class GameRulesTest : public QObject {
    Q_OBJECT

   private slots:
    void analyzeSelection_requiresAtLeastThree();
    void applyMove_removesWholeConnectedGroup();
    void collapse_dropsTilesBeforeShiftingColumnsLeft();
    void collapse_keepsNonEmptyColumnOrderStable();
    void hasMoves_usesThreeTileThreshold();
};

void GameRulesTest::analyzeSelection_requiresAtLeastThree() {
    const auto grid = makeGrid({
        {0, 0, 1},
        {2, 1, 1},
        {3, 4, 5},
    });

    const auto pairAnalysis = GameRules::analyzeSelection(grid, 0, 0, 3);
    QCOMPARE(pairAnalysis.groupSize, 2);
    QVERIFY(!pairAnalysis.valid);

    const auto tripleAnalysis = GameRules::analyzeSelection(grid, 0, 2, 3);
    QCOMPARE(tripleAnalysis.groupSize, 3);
    QVERIFY(tripleAnalysis.valid);
}

void GameRulesTest::applyMove_removesWholeConnectedGroup() {
    const auto grid = makeGrid({
        {1, 1, 2, 3},
        {1, 2, 2, 3},
        {4, 2, 5, 3},
        {4, 6, 5, 7},
    });

    const auto result = GameRules::applyMove(grid, 1, 1, 3);

    QVERIFY(result.valid);
    QCOMPARE(result.removedCount, 4);
    QCOMPARE(result.grid, makeGrid({
                              {1, -1, -1, 3},
                              {1, -1, -1, 3},
                              {4, 1, 5, 3},
                              {4, 6, 5, 7},
                          }));
}

void GameRulesTest::collapse_dropsTilesBeforeShiftingColumnsLeft() {
    const auto grid = makeGrid({
        {-1, -1, -1, -1},
        {1, -1, 2, -1},
        {-1, -1, 3, -1},
        {4, -1, 5, 6},
    });

    const auto collapsed = GameRules::collapse(grid);

    QCOMPARE(collapsed, makeGrid({
                            {-1, -1, -1, -1},
                            {-1, 2, -1, -1},
                            {1, 3, -1, -1},
                            {4, 5, 6, -1},
                        }));
}

void GameRulesTest::collapse_keepsNonEmptyColumnOrderStable() {
    const auto grid = makeGrid({
        {-1, -1, -1, -1, -1},
        {1, -1, 2, -1, 3},
        {4, -1, 5, -1, 6},
    });

    const auto collapsed = GameRules::collapse(grid);

    QCOMPARE(collapsed, makeGrid({
                            {-1, -1, -1, -1, -1},
                            {1, 2, 3, -1, -1},
                            {4, 5, 6, -1, -1},
                        }));
}

void GameRulesTest::hasMoves_usesThreeTileThreshold() {
    const auto noMoveGrid = makeGrid({
        {0, 0, 1},
        {2, 3, 1},
        {4, 5, 6},
    });
    QVERIFY(!GameRules::hasMoves(noMoveGrid, 3));

    const auto moveGrid = makeGrid({
        {0, 0, 1},
        {2, 1, 1},
        {4, 5, 6},
    });
    QVERIFY(GameRules::hasMoves(moveGrid, 3));
}

QTEST_APPLESS_MAIN(GameRulesTest)

#include "test_gamerules.moc"
