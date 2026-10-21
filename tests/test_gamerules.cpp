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
    void functions_handleEmptyAndIrregularInput();
    void findFirstValidSelection_returnsAPlayableGroup();
    void applyMove_removesWholeConnectedGroup();
    void applyMove_andPlanMove_preserveBoardForInvalidSelections();
    void collapse_dropsTilesBeforeShiftingColumnsLeft();
    void collapse_keepsNonEmptyColumnOrderStable();
    void hasMoves_usesThreeTileThreshold();
    void planMove_exposesRemovalFallAndShiftPhases();
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

void GameRulesTest::functions_handleEmptyAndIrregularInput() {
    const GameGrid emptyGrid;
    const auto emptyAnalysis = GameRules::analyzeSelection(emptyGrid, 0, 0, 3);
    QVERIFY(!emptyAnalysis.valid);
    QCOMPARE(emptyAnalysis.groupSize, 0);
    QVERIFY(GameRules::collectGroup(emptyGrid, 0, 0).isEmpty());

    const auto emptyFirst = GameRules::findFirstValidSelection(emptyGrid, 3);
    QVERIFY(!emptyFirst.valid);
    QCOMPARE(GameRules::collapseDown(emptyGrid), emptyGrid);
    QCOMPARE(GameRules::shiftColumnsLeft(emptyGrid), emptyGrid);

    const auto irregularGrid = makeGrid({
        {0, 1},
        {2},
    });
    QVERIFY(!GameRules::hasMoves(irregularGrid, 3));
    QVERIFY(GameRules::collectGroup(irregularGrid, 0, 0).isEmpty());
    QCOMPARE(GameRules::collapseDown(irregularGrid), irregularGrid);
    QCOMPARE(GameRules::shiftColumnsLeft(irregularGrid), irregularGrid);

    const auto emptyCellGrid = makeGrid({
        {-1, 1},
        {2, 3},
    });
    QVERIFY(GameRules::collectGroup(emptyCellGrid, 0, 0).isEmpty());
    QVERIFY(GameRules::collectGroup(emptyCellGrid, -1, 0).isEmpty());

    const auto removed = GameRules::removeGroup(makeGrid({
                                                    {1, 2},
                                                    {3, 4},
                                                }),
                                                {{0, 0}, {9, 9}});
    QCOMPARE(removed, makeGrid({
                          {-1, 2},
                          {3, 4},
                      }));

    QVERIFY(GameRules::buildFallMotions(emptyGrid, emptyGrid).isEmpty());
    QVERIFY(GameRules::buildFallMotions(irregularGrid, irregularGrid).isEmpty());
    QVERIFY(GameRules::buildShiftMotions(emptyGrid, emptyGrid).isEmpty());
    QVERIFY(GameRules::buildShiftMotions(irregularGrid, irregularGrid).isEmpty());
}

void GameRulesTest::findFirstValidSelection_returnsAPlayableGroup() {
    const auto grid = makeGrid({
        {0, 1, 1, 2},
        {3, 1, 4, 5},
        {6, 7, 8, 8},
    });

    const auto analysis = GameRules::findFirstValidSelection(grid, 3);

    QVERIFY(analysis.valid);
    QCOMPARE(analysis.groupSize, 3);
    QCOMPARE(analysis.group[0].row, 0);
    QCOMPARE(analysis.group[0].col, 1);
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

void GameRulesTest::applyMove_andPlanMove_preserveBoardForInvalidSelections() {
    const auto grid = makeGrid({
        {0, 0, 1},
        {2, 3, 1},
        {4, 5, 6},
    });

    const auto move = GameRules::applyMove(grid, 0, 0, 3);
    QVERIFY(!move.valid);
    QCOMPARE(move.removedCount, 0);
    QCOMPARE(move.grid, grid);
    QVERIFY(!move.hasMoves);

    const auto plan = GameRules::planMove(grid, 0, 0, 3);
    QVERIFY(!plan.valid);
    QCOMPARE(plan.removedCount, 0);
    QCOMPARE(plan.afterRemovalGrid, grid);
    QCOMPARE(plan.afterFallGrid, grid);
    QCOMPARE(plan.afterShiftGrid, grid);
    QVERIFY(plan.fallMotions.isEmpty());
    QVERIFY(plan.shiftMotions.isEmpty());
    QVERIFY(!plan.hasMoves);
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

void GameRulesTest::planMove_exposesRemovalFallAndShiftPhases() {
    const auto grid = makeGrid({
        {0, 1, -1, 2},
        {3, 1, 4, 3},
        {5, 1, -1, 5},
        {6, 1, 7, 9},
    });

    const auto plan = GameRules::planMove(grid, 0, 1, 3);

    QVERIFY(plan.valid);
    QCOMPARE(plan.removedCount, 4);
    QCOMPARE(plan.afterRemovalGrid, makeGrid({
                                        {0, -1, -1, 2},
                                        {3, -1, 4, 3},
                                        {5, -1, -1, 5},
                                        {6, -1, 7, 9},
                                    }));
    QCOMPARE(plan.afterFallGrid, makeGrid({
                                     {0, -1, -1, 2},
                                     {3, -1, -1, 3},
                                     {5, -1, 4, 5},
                                     {6, -1, 7, 9},
                                 }));
    QCOMPARE(plan.afterShiftGrid, makeGrid({
                                      {0, -1, 2, -1},
                                      {3, -1, 3, -1},
                                      {5, 4, 5, -1},
                                      {6, 7, 9, -1},
                                  }));
    QCOMPARE(plan.fallMotions.size(), 1);
    QCOMPARE(plan.shiftMotions.size(), 6);
}

QTEST_APPLESS_MAIN(GameRulesTest)

#include "test_gamerules.moc"
