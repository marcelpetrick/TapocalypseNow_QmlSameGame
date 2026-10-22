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
    void setters_emitOnlyOnValueChanges();
    void reset_withInvalidDimensionsProducesGameOverBoard();
    void privateHelpers_coverConversionAndHintBranches();
    void removeGroup_runsFullMoveFlowAndScores();
    void removeGroup_ignoresOutOfBoundsBusyEmptyAndGameOverClicks();
    void removeGroup_handlesNoFallAndStillPlayableBoards();
    void pendingSelection_canBecomeInvalidBeforeResolution();
    void reset_cancelsPendingTimers();
    void invalidSelection_shakesThenClears();
    void idleHint_restartsAfterUserAction();
    void hintCallbacks_handleBusyAndNoMoveBoards();
    void internalCallbacks_coverDefensiveBranches();
    void endRun_endsPlayableRunOnceAndIgnoresBusyOrFinishedBoards();
    void benchmarkStep_playsFirstGroupAndRestartsFinishedGames();
    void hover_previewsRemovableGroupsOnly();
    void fallPhase_neverRevealsHiddenSourcesOnTheOldGrid();
};

void GameBoardControllerTest::setters_emitOnlyOnValueChanges() {
    GameBoardController controller;
    QSignalSpy rowsSpy(&controller, &GameBoardController::rowsChanged);
    QSignalSpy colsSpy(&controller, &GameBoardController::colsChanged);
    QSignalSpy groupSpy(&controller, &GameBoardController::minimumGroupSizeChanged);

    controller.setRows(controller.rows());
    controller.setCols(controller.cols());
    controller.setMinimumGroupSize(controller.minimumGroupSize());
    QCOMPARE(rowsSpy.count(), 0);
    QCOMPARE(colsSpy.count(), 0);
    QCOMPARE(groupSpy.count(), 0);

    controller.setRows(8);
    controller.setCols(9);
    controller.setMinimumGroupSize(4);
    QCOMPARE(controller.rows(), 8);
    QCOMPARE(controller.cols(), 9);
    QCOMPARE(controller.minimumGroupSize(), 4);
    QCOMPARE(rowsSpy.count(), 1);
    QCOMPARE(colsSpy.count(), 1);
    QCOMPARE(groupSpy.count(), 1);
}

void GameBoardControllerTest::reset_withInvalidDimensionsProducesGameOverBoard() {
    GameBoardController controller;
    controller.setRows(0);
    controller.setCols(4);

    controller.reset();

    QVERIFY(controller.gridState().isEmpty());
    QVERIFY(controller.gameOver());

    controller.setRows(1);
    controller.setCols(1);
    controller.setMinimumGroupSize(3);
    controller.reset();
    QVERIFY(!controller.gridState().isEmpty());
    QVERIFY(controller.gameOver());

    controller.setRows(3);
    controller.setCols(3);
    controller.setMinimumGroupSize(3);
    controller.reset();
    QVERIFY(!controller.gridState().isEmpty());
    QVERIFY(!controller.gameOver());
}

void GameBoardControllerTest::privateHelpers_coverConversionAndHintBranches() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(1, 1, 1, 1, 1);
    controller.setHintTimingsForTesting(1000, 1000);

    controller.setGridState(makeGrid({
        {0, 1, 1},
        {2, 1, 4},
        {5, 6, 7},
    }));
    QCOMPARE(controller.grid().size(), 3);

    controller.hideMotionSources({});
    QVERIFY(controller.hiddenSet().isEmpty());

    controller.buildAnimatedTiles({}, 123);
    QVERIFY(controller.movingTiles().isEmpty());

    controller.m_gameOver = false;
    controller.m_busy = false;
    controller.m_gridState = makeGrid({
        {0, 0, 1},
        {2, 3, 1},
        {4, 5, 6},
    });
    QVERIFY(!controller.triggerHintFlash());
    QVERIFY(controller.hintSet().isEmpty());

    controller.m_busy = true;
    controller.restartHintIdleTimer();
    controller.m_busy = false;
    controller.m_gameOver = true;
    controller.restartHintIdleTimer();
    controller.m_gameOver = false;
    controller.m_gridState = {};
    controller.restartHintIdleTimer();
    controller.m_gridState = makeGrid({
        {0, 0, 1},
        {2, 3, 1},
        {4, 5, 6},
    });
    controller.restartHintIdleTimer();
    controller.m_gridState = makeGrid({
        {0, 1, 1},
        {2, 1, 4},
        {5, 6, 7},
    });
    controller.restartHintIdleTimer();

    controller.setRows(2);
    controller.setCols(2);
    const auto playable = controller.buildPlayableGrid();
    QVERIFY(!playable.isEmpty());

    controller.setRows(1);
    controller.setCols(1);
    controller.setMinimumGroupSize(3);
    const auto unplayable = controller.buildPlayableGrid();
    QCOMPARE(unplayable.size(), 1);
    QCOMPARE(unplayable.first().size(), 1);

    controller.setRows(2);
    controller.setCols(0);
    QVERIFY(controller.buildPlayableGrid().isEmpty());

    controller.m_gridState = makeGrid({
        {0, 1},
        {2, 3},
    });
    QVERIFY(controller.inBounds(0, 0));
    QVERIFY(!controller.inBounds(-1, 0));
    QVERIFY(!controller.inBounds(2, 0));
    QVERIFY(!controller.inBounds(0, -1));
    QVERIFY(!controller.inBounds(0, 2));

    controller.m_gridState = {};
    QVERIFY(!controller.inBounds(0, 0));

    controller.setGridForTesting({});
    QVERIFY(controller.gameOver());
}

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
    QSignalSpy selectedSpy(&controller, &GameBoardController::groupSelected);
    QSignalSpy landedSpy(&controller, &GameBoardController::tilesLanded);
    QSignalSpy rejectedSpy(&controller, &GameBoardController::moveRejected);

    controller.removeGroup(0, 1);

    QVERIFY(controller.busy());
    QCOMPARE(controller.popSet().size(), 4);
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(selectedSpy.takeFirst().at(0).toInt(), 4);

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
    // Column 2 falls, then columns shift left: two landings, no rejection.
    QCOMPARE(landedSpy.count(), 2);
    QCOMPARE(rejectedSpy.count(), 0);
}

void GameBoardControllerTest::removeGroup_ignoresOutOfBoundsBusyEmptyAndGameOverClicks() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(50, 50, 50, 50, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setGridForTesting(makeGrid({
        {-1, 1, 1},
        {2, 1, 3},
        {4, 5, 6},
    }));

    controller.removeGroup(-1, 0);
    QCOMPARE(controller.score(), 0);

    controller.removeGroup(0, 0);
    QCOMPARE(controller.score(), 0);

    controller.removeGroup(0, 1);
    QVERIFY(controller.busy());
    controller.removeGroup(1, 0);
    QCOMPARE(controller.moves(), 0);

    controller.setGridForTesting(makeGrid({
        {0, 0, 1},
        {2, 3, 1},
        {4, 5, 6},
    }));
    QVERIFY(controller.gameOver());
    controller.removeGroup(0, 0);
    QCOMPARE(controller.moves(), 0);
}

void GameBoardControllerTest::removeGroup_handlesNoFallAndStillPlayableBoards() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(1, 1, 1, 1, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setGridForTesting(makeGrid({
        {0, 1, 2},
        {3, 4, 5},
        {6, 6, 6},
        {1, 1, 1},
    }));

    controller.removeGroup(2, 0);

    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 100);
    QVERIFY(!controller.gameOver());
    QCOMPARE(controller.moves(), 1);
    QCOMPARE(controller.score(), 9);
    QCOMPARE(controller.gridState(), makeGrid({
                                         {-1, -1, -1},
                                         {0, 1, 2},
                                         {3, 4, 5},
                                         {1, 1, 1},
                                     }));
}

void GameBoardControllerTest::pendingSelection_canBecomeInvalidBeforeResolution() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(5, 5, 5, 5, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setGridForTesting(makeGrid({
        {0, 1, 1},
        {2, 1, 4},
        {5, 6, 7},
    }));

    controller.removeGroup(0, 1);
    QVERIFY(controller.busy());
    controller.setMinimumGroupSize(4);

    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 100);
    QCOMPARE(controller.moves(), 0);
    QCOMPARE(controller.score(), 0);
    QVERIFY(!controller.gameOver());
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

    QSignalSpy rejectedSpy(&controller, &GameBoardController::moveRejected);
    QSignalSpy selectedSpy(&controller, &GameBoardController::groupSelected);

    controller.removeGroup(0, 0);

    QCOMPARE(rejectedSpy.count(), 1);
    QCOMPARE(selectedSpy.count(), 0);
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

void GameBoardControllerTest::hintCallbacks_handleBusyAndNoMoveBoards() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(100, 100, 100, 100, 1);
    controller.setHintTimingsForTesting(1, 1);
    controller.setGridForTesting(makeGrid({
        {0, 1, 1},
        {2, 1, 4},
        {5, 6, 7},
    }));

    controller.removeGroup(0, 1);
    QVERIFY(controller.busy());
    QVERIFY(QMetaObject::invokeMethod(&controller, "onIdleHintTimer"));
    QCOMPARE(controller.hintPulseEpoch(), 0);

    controller.setGridForTesting(makeGrid({
        {0, 0, 1},
        {2, 3, 1},
        {4, 5, 6},
    }));
    QVERIFY(controller.gameOver());
    QVERIFY(QMetaObject::invokeMethod(&controller, "onHintRepeatTimer"));
    QCOMPARE(controller.hintPulseEpoch(), 0);
}

void GameBoardControllerTest::internalCallbacks_coverDefensiveBranches() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(1, 1, 1, 1, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setGridForTesting(makeGrid({
        {0, 1, 1},
        {2, 1, 4},
        {5, 6, 7},
    }));

    controller.m_phaseHasMoves = false;
    controller.m_pendingMovePlan = MovePlan{};
    controller.onRemoveHoldTimer();
    QVERIFY(controller.gameOver());

    controller.setGridForTesting(makeGrid({
        {0, 1, 1},
        {2, 1, 4},
        {5, 6, 7},
    }));
    controller.removeGroup(0, 1);
    controller.onRemovePreviewTimer();
    controller.onFallTimer();
    controller.onShiftTimer();
    controller.onShakeClearTimer();

    controller.setGridForTesting(makeGrid({
        {1, 1, 1},
        {2, 3, 4},
        {5, 5, 5},
    }));
    controller.removeGroup(0, 0);
    controller.onRemovePreviewTimer();
    controller.onRemoveHoldTimer();
    QVERIFY(!controller.busy());
}

void GameBoardControllerTest::endRun_endsPlayableRunOnceAndIgnoresBusyOrFinishedBoards() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(50, 50, 50, 50, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setGridForTesting(makeGrid({
        {1, 1, 1, 2},
        {3, 4, 4, 4},
    }));
    QSignalSpy endedSpy(&controller, &GameBoardController::gameEnded);

    controller.endRun();  // no move yet
    QVERIFY(!controller.gameOver());
    QCOMPARE(endedSpy.count(), 0);

    controller.removeGroup(0, 0);
    QVERIFY(controller.busy());
    controller.endRun();
    QCOMPARE(endedSpy.count(), 0);
    QVERIFY(!controller.gameOver());

    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 1000);
    QCOMPARE(controller.score(), 9);
    QVERIFY(!controller.gameOver());

    controller.endRun();
    QVERIFY(controller.gameOver());
    QCOMPARE(endedSpy.count(), 1);
    QCOMPARE(endedSpy.takeFirst().at(0).toInt(), 9);

    controller.endRun();
    QCOMPARE(endedSpy.count(), 0);
    const GameGrid frozen = controller.gridState();
    controller.removeGroup(1, 1);
    QCOMPARE(controller.gridState(), frozen);
}

void GameBoardControllerTest::benchmarkStep_playsFirstGroupAndRestartsFinishedGames() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(1, 1, 1, 1, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setRows(4);
    controller.setCols(4);
    controller.setGridForTesting(makeGrid({
        {1, 2, 3},
        {4, 4, 4},
    }));

    controller.benchmarkStep();
    QVERIFY(controller.busy());
    controller.benchmarkStep();
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 1000);
    QCOMPARE(controller.moves(), 1);
    QVERIFY(controller.gameOver());

    controller.benchmarkStep();
    QVERIFY(!controller.gameOver());
    QCOMPARE(controller.moves(), 0);
    QCOMPARE(controller.gridState().size(), 4);

    controller.m_gameOver = false;
    controller.m_gridState = makeGrid({{1, 2}});
    controller.benchmarkStep();
    QCOMPARE(controller.gridState().size(), 4);
}

void GameBoardControllerTest::hover_previewsRemovableGroupsOnly() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(20, 20, 20, 20, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    controller.setGridForTesting(makeGrid({
        {1, 1, 1, 2},
        {3, 4, 4, 4},
        {5, 6, 7, 8},
    }));
    QSignalSpy changed(&controller, &GameBoardController::hoverSetChanged);

    controller.setHoverCell(0, 0);
    QCOMPARE(controller.hoverSet().size(), 3);
    controller.setHoverCell(0, 1);
    QCOMPARE(changed.count(), 1);  // same group: no re-emit

    controller.setHoverCell(0, 3);
    QVERIFY(controller.hoverSet().isEmpty());  // a single tile is not removable
    controller.leaveHoverCell(1, 1);           // leaving a cell we are not on changes nothing
    controller.setHoverCell(1, 2);
    QCOMPARE(controller.hoverSet().size(), 3);
    controller.leaveHoverCell(1, 2);
    QVERIFY(controller.hoverSet().isEmpty());

    // Hidden while a move resolves, re-evaluated on the settled board under the same pointer position.
    controller.setHoverCell(0, 0);
    controller.removeGroup(0, 0);
    QVERIFY(controller.hoverSet().isEmpty());
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 1000);
    QCOMPARE(controller.gridState()[0][0], -1);
    QVERIFY(controller.hoverSet().isEmpty());
    controller.setHoverCell(1, 1);
    QCOMPARE(controller.hoverSet().size(), 3);

    // Raising the threshold re-evaluates the preview without any grid change.
    controller.setMinimumGroupSize(4);
    QVERIFY(controller.hoverSet().isEmpty());
    controller.setMinimumGroupSize(3);
    QCOMPARE(controller.hoverSet().size(), 3);

    controller.endRun();
    QVERIFY(controller.hoverSet().isEmpty());
}

void GameBoardControllerTest::fallPhase_neverRevealsHiddenSourcesOnTheOldGrid() {
    GameBoardController controller;
    controller.setAnimationTimingsForTesting(1, 1, 1, 1, 1);
    controller.setHintTimingsForTesting(1000, 1000);
    const GameGrid grid = makeGrid({
        {1, 2, 3},
        {4, 4, 4},
        {5, 6, 7},
    });
    controller.setGridForTesting(grid);
    const auto plan = GameRules::planMove(grid, 1, 0, 3);
    QVERIFY(!plan.fallMotions.isEmpty());

    // Whenever the overlays are dropped, the grid must already show where the tiles landed.
    QVector<GameGrid> gridsWhenUnhidden;
    connect(&controller, &GameBoardController::hiddenSetChanged, &controller, [&] {
        if (controller.hiddenSet().isEmpty() && controller.busy()) gridsWhenUnhidden.push_back(controller.gridState());
    });

    controller.removeGroup(1, 0);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 1000);

    QVERIFY(!gridsWhenUnhidden.isEmpty());
    for (const auto& seen : gridsWhenUnhidden) QVERIFY(seen == plan.afterFallGrid || seen == plan.afterShiftGrid);
}

QTEST_GUILESS_MAIN(GameBoardControllerTest)

#include "test_gameboardcontroller.moc"
