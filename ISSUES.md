# List of issues, which have to be mended - since no collaboration board is used, I list them here
* remove the "reset highscore"-button and functionality
* animations for the collapse are missing
* game logic is sometimes flawed: all groups of three similar tiles shall be removable
* add sound effects?
* when can you enter the highscore?
* option to abort the game as is: if sufficent score, then enter to highscore
* logging of the console.log in QtCreator missing - by direct start it logs to stdout

-----

## Worst Problems

1. qml/GameBoard.qml:76: reset() clears visible state but does not stop removeTimer or clear pending removals, so clicking New Game during the pop delay can let the old move mutate the
    new board when the timer fires at qml/GameBoard.qml:223. Add explicit timer cancellation and a real game-state machine.
2. qml/GameBoard.qml:75: board state, flood fill, collapse, scoring, animation staging, and rendering all live in one item. That makes it hard to test, hard to reuse, and easy to break
    while tweaking visuals. Add a separate model/controller layer, ideally with unit-tested pure logic.
3. README.md:62: no unit tests, no QML tests, no game-rule regression coverage, no persistence tests. For a puzzle game, that is a major quality hole. Add Qt Test coverage for flood
    fill, collapse, move detection, highscore qualification, and popup flow.
4. src/HighScoreManager.cpp:71: any score qualifies while fewer than 10 entries exist, including 0, and ties never qualify even though the sort has a tie rule. Add an explicit product
    rule for zero-score/aborted runs and make qualification consistent with sort order.
5. qml/ScoreDialog.qml:20: openForBrowsing() just opens the popup; it does not refresh scoresView.model, and Reset Highscores in qml/main.qml:93 does not notify the dialog. Add change
    notifications from C++ and bind the list to a real model instead of one-off loadScores() calls.
6. src/main.cpp:7: the app never sets organization/app name, yet storage depends on QStandardPaths::AppDataLocation at src/HighScoreManager.cpp:13. That makes the save path brittle and
    platform-dependent. Add QCoreApplication::setOrganizationName, setApplicationName, and a documented data migration story.
7. qml/main.qml:13: the window starts at 1920x1080, the board width is hard-coded to 55%, and the toolbar is a single top row. This will degrade quickly on small laptop screens and high-
    DPI setups. Add minimum sizes, adaptive layouts, and a board sizing strategy driven by available space.
8. qml/ScoreDialog.qml:83: Save does not close the popup, does not clear the input, and does not prevent repeated submissions of the same score. Add submit-once protection, input reset,
    enter-to-submit, and a clear post-save state.
9. README.md:100: the README explicitly says the quick start is “auto generated, not tested,” and packaging is left to manual windeployqt/macdeployqt. Add a tested build matrix, CI, and
    at least one reproducible desktop packaging path.
10. ISSUES.md:1: it mentions animation polish and sound, but not state races, missing tests, stale model bindings, or persistence semantics. Add a proper backlog with severity,
    reproduction steps, and architecture tasks, not just feature notes.

### What Should Be Added

The highest-value additions are: a tested game-logic layer, automated tests, a proper highscore model with signals, explicit app metadata, CI/build verification, responsive layout work,
and a small state machine for game lifecycle (idle, animating, gameOver, submittingScore).
