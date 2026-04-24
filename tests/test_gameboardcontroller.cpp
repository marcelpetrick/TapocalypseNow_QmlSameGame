#include <QtTest>

#include "GameBoardController.h"

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

class GameBoardControllerTest : public QObject {
    Q_OBJECT

   private slots:
    void removeGroup_runsFullMoveFlowAndScores();
    void reset_cancelsPendingTimers();
    void invalidSelection_shakesThenClears();
    void idleHint_restartsAfterUserAction();
};

void GameBoardControllerTest::removeGroup_runsFullMoveFlowAndScores() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(1, 1, 1, 1, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setGridForTesting(makeGrid({
        {0, 1, -1, 2},
        {3, 1, 4, 3},
        {5, 1, -1, 5},
        {6, 1, 7, 9},
    }));

    QSignalSpy endedSpy(&controller, &GameBoardController::gameEnded);

    controller.removeGroup(0, 1);

    QVERIFY(controller.busy());
    QCOMPARE(controller.popSet().size(), 4);

    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 100);

    QCOMPARE(controller.gridState(), makeGrid({
                                     {0, -1, 2, -1},
                                     {3, -1, 3, -1},
                                     {5, 4, 5, -1},
                                     {6, 7, 9, -1},
                                 }));
    QCOMPARE(controller.score(), 16);
    QCOMPARE(controller.moves(), 1);
    QVERIFY(controller.gameOver());
    QCOMPARE(endedSpy.count(), 1);
    QCOMPARE(endedSpy.takeFirst().at(0).toInt(), 16);
}

void GameBoardControllerTest::reset_cancelsPendingTimers() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(50, 50, 50, 50, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setGridForTesting(makeGrid({
        {0, 1, -1, 2},
        {3, 1, 4, 3},
        {5, 1, -1, 5},
        {6, 1, 7, 9},
    }));

    controller.removeGroup(0, 1);
    QVERIFY(controller.busy());

    const auto replacement = makeGrid({
        {0, 0, 1},
        {2, 1, 1},
        {4, 5, 6},
    });

    controller.setGridForTesting(replacement);

    QTest::qWait(250);

    QCOMPARE(controller.gridState(), replacement);
    QCOMPARE(controller.score(), 0);
    QCOMPARE(controller.moves(), 0);
    QVERIFY(!controller.busy());
}

void GameBoardControllerTest::invalidSelection_shakesThenClears() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(1, 1, 1, 1, 5);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setGridForTesting(makeGrid({
        {0, 0, 1},
        {2, 1, 1},
        {4, 5, 6},
    }));

    controller.removeGroup(0, 0);

    QVERIFY(controller.shakeSet().contains("0,0"));
    QTRY_VERIFY_WITH_TIMEOUT(controller.shakeSet().isEmpty(), 100);
    QCOMPARE(controller.score(), 0);
    QCOMPARE(controller.moves(), 0);
}

void GameBoardControllerTest::idleHint_restartsAfterUserAction() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(1, 1, 1, 1, 1);
    controller.setHintTimingsForTesting(40, 20);
    controller.setGridForTesting(makeGrid({
        {0, 0, 1},
        {2, 1, 1},
        {4, 5, 6},
    }));

    QTRY_VERIFY_WITH_TIMEOUT(controller.hintPulseEpoch() >= 2, 200);

    const int pulseEpoch = controller.hintPulseEpoch();
    controller.registerUserAction();

    QTest::qWait(10);
    QCOMPARE(controller.hintPulseEpoch(), pulseEpoch);

    QTRY_VERIFY_WITH_TIMEOUT(controller.hintPulseEpoch() > pulseEpoch, 200);
}

QTEST_GUILESS_MAIN(GameBoardControllerTest)

#include "test_gameboardcontroller.moc"
