#include <QtTest>

#include "GameLogicBridge.h"

namespace {

QVariantList makeGrid(std::initializer_list<std::initializer_list<int>> rows) {
    QVariantList grid;
    for (const auto& rowValues : rows) {
        QVariantList row;
        for (const auto value : rowValues) {
            row.push_back(value);
        }
        grid.push_back(row);
    }
    return grid;
}

}  // namespace

class GameLogicBridgeTest : public QObject {
    Q_OBJECT

   private slots:
    void analyzeSelection_reportsValidityAndGroupSize();
    void findFirstValidSelection_reportsMissingAndPresentGroups();
    void applyMove_returnsCollapsedGrid();
    void applyMove_reportsInvalidSelections();
    void planMove_exposesAnimationPhases();
    void methods_handleEmptyGrids();
    void hasMoves_tracksPlayableState();
};

void GameLogicBridgeTest::analyzeSelection_reportsValidityAndGroupSize() {
    GameLogicBridge bridge;
    const auto grid = makeGrid({
        {0, 1, 1},
        {2, 1, 3},
        {4, 5, 6},
    });

    const auto valid = bridge.analyzeSelection(grid, 0, 1, 3);
    QVERIFY(valid.value("valid").toBool());
    QCOMPARE(valid.value("groupSize").toInt(), 3);
    QVERIFY(valid.value("selection").toMap().value("0,1").toBool());

    const auto invalid = bridge.analyzeSelection(grid, 2, 2, 3);
    QVERIFY(!invalid.value("valid").toBool());
    QCOMPARE(invalid.value("groupSize").toInt(), 1);
}

void GameLogicBridgeTest::findFirstValidSelection_reportsMissingAndPresentGroups() {
    GameLogicBridge bridge;

    const auto noMove = bridge.findFirstValidSelection(makeGrid({
                                                           {0, 0, 1},
                                                           {2, 3, 1},
                                                           {4, 5, 6},
                                                       }),
                                                       3);
    QVERIFY(!noMove.value("valid").toBool());
    QCOMPARE(noMove.value("groupSize").toInt(), 0);
    QVERIFY(noMove.value("selection").toMap().isEmpty());

    const auto move = bridge.findFirstValidSelection(makeGrid({
                                                         {0, 1, 1},
                                                         {2, 1, 4},
                                                         {5, 6, 7},
                                                     }),
                                                     3);
    QVERIFY(move.value("valid").toBool());
    QCOMPARE(move.value("groupSize").toInt(), 3);
    QVERIFY(move.value("selection").toMap().value("0,1").toBool());
}

void GameLogicBridgeTest::applyMove_returnsCollapsedGrid() {
    GameLogicBridge bridge;
    const auto move = bridge.applyMove(makeGrid({
                                            {1, 1, 2, 3},
                                            {1, 2, 2, 3},
                                            {4, 2, 5, 3},
                                            {4, 6, 5, 7},
                                        }),
                                        1, 1, 3);

    QVERIFY(move.value("valid").toBool());
    QCOMPARE(move.value("removedCount").toInt(), 4);
    QVERIFY(move.value("hasMoves").toBool());

    const auto grid = move.value("grid").toList();
    QCOMPARE(grid[2].toList()[1].toInt(), 1);
    QCOMPARE(grid[0].toList()[1].toInt(), -1);
}

void GameLogicBridgeTest::applyMove_reportsInvalidSelections() {
    GameLogicBridge bridge;
    const auto move = bridge.applyMove(makeGrid({
                                            {0, 0, 1},
                                            {2, 3, 1},
                                            {4, 5, 6},
                                        }),
                                        0, 0, 3);

    QVERIFY(!move.value("valid").toBool());
    QCOMPARE(move.value("removedCount").toInt(), 0);
    QVERIFY(!move.value("hasMoves").toBool());
    QVERIFY(!move.value("selection").toMap().isEmpty());
}

void GameLogicBridgeTest::planMove_exposesAnimationPhases() {
    GameLogicBridge bridge;
    const auto move = bridge.planMove(makeGrid({
                                           {0, 1, -1, 2},
                                           {3, 1, 4, 3},
                                           {5, 1, -1, 5},
                                           {6, 1, 7, 9},
                                       }),
                                       0, 1, 3);

    QVERIFY(move.value("valid").toBool());
    QCOMPARE(move.value("removedCount").toInt(), 4);
    QCOMPARE(move.value("fallMotions").toList().size(), 1);
    QCOMPARE(move.value("shiftMotions").toList().size(), 6);
    QCOMPARE(move.value("afterShiftGrid").toList()[0].toList()[2].toInt(), 2);

    const auto invalid = bridge.planMove(makeGrid({
                                              {0, 0, 1},
                                              {2, 3, 1},
                                              {4, 5, 6},
                                          }),
                                          0, 0, 3);
    QVERIFY(!invalid.value("valid").toBool());
    QVERIFY(invalid.value("fallMotions").toList().isEmpty());
    QVERIFY(invalid.value("shiftMotions").toList().isEmpty());
}

void GameLogicBridgeTest::methods_handleEmptyGrids() {
    GameLogicBridge bridge;
    const QVariantList emptyGrid;

    const auto analysis = bridge.analyzeSelection(emptyGrid, 0, 0, 3);
    QVERIFY(!analysis.value("valid").toBool());
    QVERIFY(analysis.value("selection").toMap().isEmpty());

    const auto move = bridge.applyMove(emptyGrid, 0, 0, 3);
    QVERIFY(!move.value("valid").toBool());
    QVERIFY(move.value("grid").toList().isEmpty());

    const auto plan = bridge.planMove(emptyGrid, 0, 0, 3);
    QVERIFY(!plan.value("valid").toBool());
    QVERIFY(plan.value("afterShiftGrid").toList().isEmpty());
    QVERIFY(plan.value("fallMotions").toList().isEmpty());

    QVERIFY(!bridge.hasMoves(emptyGrid, 3));
}

void GameLogicBridgeTest::hasMoves_tracksPlayableState() {
    GameLogicBridge bridge;
    QVERIFY(bridge.hasMoves(makeGrid({
                                {0, 1, 1},
                                {2, 1, 4},
                                {5, 6, 7},
                            }),
                            3));
    QVERIFY(!bridge.hasMoves(makeGrid({
                                 {0, 0, 1},
                                 {2, 3, 1},
                                 {4, 5, 6},
                             }),
                             3));
}

QTEST_APPLESS_MAIN(GameLogicBridgeTest)

#include "test_gamelogicbridge.moc"
