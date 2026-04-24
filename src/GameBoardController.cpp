#include "GameBoardController.h"

#include <QRandomGenerator>

namespace {

QVariantList toVariantGrid(const GameGrid& grid) {
    QVariantList rawGrid;
    rawGrid.reserve(grid.size());

    for (const auto& row : grid) {
        QVariantList rawRow;
        rawRow.reserve(row.size());
        for (const auto cell : row) {
            rawRow.push_back(cell);
        }
        rawGrid.push_back(rawRow);
    }

    return rawGrid;  // GCOVR_EXCL_LINE
}  // GCOVR_EXCL_LINE

QVariantMap toSelectionMap(const QVector<BoardPosition>& group) {
    QVariantMap selection;
    for (const auto& position : group) {
        selection.insert(QStringLiteral("%1,%2").arg(position.row).arg(position.col), true);
    }
    return selection;  // GCOVR_EXCL_LINE
}  // GCOVR_EXCL_LINE

QVariantList toMotions(const QVector<TileMotion>& motions, int durationMs) {
    QVariantList rawMotions;
    rawMotions.reserve(motions.size());

    for (const auto& motion : motions) {
        QVariantMap item;
        item.insert("color", motion.color);
        item.insert("fromRow", motion.from.row);
        item.insert("fromCol", motion.from.col);
        item.insert("toRow", motion.to.row);
        item.insert("toCol", motion.to.col);
        item.insert("duration", durationMs);
        rawMotions.push_back(item);
    }

    return rawMotions;  // GCOVR_EXCL_LINE
}  // GCOVR_EXCL_LINE

}  // namespace

GameBoardController::GameBoardController(QObject* parent) : QObject(parent) {
    m_removePreviewTimer.setParent(this);
    m_removeHoldTimer.setParent(this);
    m_fallTimer.setParent(this);
    m_shiftTimer.setParent(this);
    m_shakeClearTimer.setParent(this);
    m_idleHintTimer.setParent(this);
    m_hintRepeatTimer.setParent(this);

    m_removePreviewTimer.setSingleShot(true);
    m_removeHoldTimer.setSingleShot(true);
    m_fallTimer.setSingleShot(true);
    m_shiftTimer.setSingleShot(true);
    m_shakeClearTimer.setSingleShot(true);
    m_idleHintTimer.setSingleShot(true);
    m_hintRepeatTimer.setSingleShot(false);

    connect(&m_removePreviewTimer, &QTimer::timeout, this, &GameBoardController::onRemovePreviewTimer);
    connect(&m_removeHoldTimer, &QTimer::timeout, this, &GameBoardController::onRemoveHoldTimer);
    connect(&m_fallTimer, &QTimer::timeout, this, &GameBoardController::onFallTimer);
    connect(&m_shiftTimer, &QTimer::timeout, this, &GameBoardController::onShiftTimer);
    connect(&m_shakeClearTimer, &QTimer::timeout, this, &GameBoardController::onShakeClearTimer);
    connect(&m_idleHintTimer, &QTimer::timeout, this, &GameBoardController::onIdleHintTimer);
    connect(&m_hintRepeatTimer, &QTimer::timeout, this, &GameBoardController::onHintRepeatTimer);
}

int GameBoardController::rows() const { return m_rows; }

void GameBoardController::setRows(int rows) {
    if (m_rows == rows) return;
    m_rows = rows;
    emit rowsChanged();
}

int GameBoardController::cols() const { return m_cols; }

void GameBoardController::setCols(int cols) {
    if (m_cols == cols) return;
    m_cols = cols;
    emit colsChanged();
}

int GameBoardController::minimumGroupSize() const { return m_minimumGroupSize; }

void GameBoardController::setMinimumGroupSize(int minimumGroupSize) {
    if (m_minimumGroupSize == minimumGroupSize) return;
    m_minimumGroupSize = minimumGroupSize;
    emit minimumGroupSizeChanged();
}

int GameBoardController::score() const { return m_score; }

int GameBoardController::moves() const { return m_moves; }

bool GameBoardController::gameOver() const { return m_gameOver; }

bool GameBoardController::busy() const { return m_busy; }

QVariantList GameBoardController::grid() const { return m_grid; }

QVariantMap GameBoardController::popSet() const { return m_popSet; }

QVariantMap GameBoardController::shakeSet() const { return m_shakeSet; }

QVariantMap GameBoardController::hiddenSet() const { return m_hiddenSet; }

QVariantList GameBoardController::movingTiles() const { return m_movingTiles; }

QVariantMap GameBoardController::hintSet() const { return m_hintSet; }

int GameBoardController::hintPulseEpoch() const { return m_hintPulseEpoch; }

const GameGrid& GameBoardController::gridState() const { return m_gridState; }

void GameBoardController::setScore(int score) {
    if (m_score == score) return;
    m_score = score;
    emit scoreChanged();
}

void GameBoardController::setMoves(int moves) {
    if (m_moves == moves) return;
    m_moves = moves;
    emit movesChanged();
}

void GameBoardController::setGameOver(bool gameOver) {
    if (m_gameOver == gameOver) return;
    m_gameOver = gameOver;
    emit gameOverChanged();
}

void GameBoardController::setBusy(bool busy) {
    if (m_busy == busy) return;
    m_busy = busy;
    emit busyChanged();
}

void GameBoardController::setGridState(const GameGrid& grid) {
    m_gridState = grid;
    m_grid = toVariantGrid(m_gridState);
    emit gridChanged();
}

void GameBoardController::setPopSet(const QVariantMap& popSet) {
    m_popSet = popSet;
    emit popSetChanged();
}

void GameBoardController::setShakeSet(const QVariantMap& shakeSet) {
    m_shakeSet = shakeSet;
    emit shakeSetChanged();
}

void GameBoardController::setHiddenSet(const QVariantMap& hiddenSet) {
    m_hiddenSet = hiddenSet;
    emit hiddenSetChanged();
}

void GameBoardController::setMovingTiles(const QVariantList& movingTiles) {
    m_movingTiles = movingTiles;
    emit movingTilesChanged();
}

void GameBoardController::setHintSet(const QVariantMap& hintSet) {
    m_hintSet = hintSet;
    emit hintSetChanged();
}

void GameBoardController::incrementHintPulseEpoch() {
    ++m_hintPulseEpoch;
    emit hintPulseEpochChanged();
}

void GameBoardController::clearSelections() {
    setPopSet({});
    setShakeSet({});
    setHintSet({});
}

void GameBoardController::clearPhaseState() {
    setHiddenSet({});
    setMovingTiles({});
    m_pendingMovePlan = MovePlan{};
    m_phaseHasMoves = false;
}

void GameBoardController::cancelTimers() {
    m_removePreviewTimer.stop();
    m_removeHoldTimer.stop();
    m_fallTimer.stop();
    m_shiftTimer.stop();
    m_shakeClearTimer.stop();
    m_idleHintTimer.stop();
    m_hintRepeatTimer.stop();
}

void GameBoardController::stopHintCycle() {
    m_idleHintTimer.stop();
    m_hintRepeatTimer.stop();
    setHintSet({});
}

void GameBoardController::restartHintIdleTimer() {
    stopHintCycle();
    if (m_busy || m_gameOver || m_gridState.isEmpty() || !GameRules::hasMoves(m_gridState, m_minimumGroupSize)) return;
    m_idleHintTimer.start(m_idleHintDelayMs);
}

bool GameBoardController::triggerHintFlash() {
    if (m_busy || m_gameOver) return false;

    const auto hint = GameRules::findFirstValidSelection(m_gridState, m_minimumGroupSize);
    if (!hint.valid) {
        stopHintCycle();
        return false;
    }

    setHintSet(toSelectionMap(hint.group));
    incrementHintPulseEpoch();
    return true;
}

void GameBoardController::hideMotionSources(const QVector<TileMotion>& motions) {
    QVariantMap nextHidden;
    for (const auto& motion : motions) {
        nextHidden.insert(QStringLiteral("%1,%2").arg(motion.from.row).arg(motion.from.col), true);
    }
    setHiddenSet(nextHidden);
}

void GameBoardController::buildAnimatedTiles(const QVector<TileMotion>& motions, int durationMs) { setMovingTiles(toMotions(motions, durationMs)); }

void GameBoardController::startResolvedMove() {
    const auto move = GameRules::planMove(m_gridState, m_pendingRow, m_pendingCol, m_minimumGroupSize);
    if (!move.valid) {
        setBusy(false);
        restartHintIdleTimer();
        return;
    }

    m_pendingMovePlan = move;
    m_phaseHasMoves = move.hasMoves;
    setPopSet({});
    setGridState(move.afterRemovalGrid);
    setScore(m_score + move.removedCount * move.removedCount);
    setMoves(m_moves + 1);
    m_removeHoldTimer.start(m_removeHoldMs);
}

void GameBoardController::applyFallPhase() {
    if (!m_pendingMovePlan.valid) {
        finishMovePhases();
        return;
    }

    if (m_pendingMovePlan.fallMotions.isEmpty()) {
        setGridState(m_pendingMovePlan.afterFallGrid);
        applyShiftPhase();
        return;
    }

    hideMotionSources(m_pendingMovePlan.fallMotions);
    buildAnimatedTiles(m_pendingMovePlan.fallMotions, m_fallAnimMs);
    m_fallTimer.start(m_fallAnimMs);
}

void GameBoardController::applyShiftPhase() {
    setGridState(m_pendingMovePlan.afterFallGrid);
    if (m_pendingMovePlan.shiftMotions.isEmpty()) {
        finishMovePhases();
        return;
    }

    hideMotionSources(m_pendingMovePlan.shiftMotions);
    buildAnimatedTiles(m_pendingMovePlan.shiftMotions, m_shiftAnimMs);
    m_shiftTimer.start(m_shiftAnimMs);
}

void GameBoardController::finishMovePhases() {
    const bool stillHasMoves = m_phaseHasMoves;

    if (m_pendingMovePlan.valid) setGridState(m_pendingMovePlan.afterShiftGrid);

    clearSelections();
    clearPhaseState();
    m_pendingRow = -1;
    m_pendingCol = -1;
    setBusy(false);

    if (!stillHasMoves) {
        setGameOver(true);
        emit gameEnded(m_score);
        return;
    }

    restartHintIdleTimer();
}

GameGrid GameBoardController::buildPlayableGrid() const {
    if (m_rows <= 0 || m_cols <= 0) return {};

    GameGrid grid;
    for (int attempts = 0; attempts < kMaxResetAttempts; ++attempts) {
        GameGrid candidate;
        candidate.reserve(m_rows);

        for (int row = 0; row < m_rows; ++row) {
            QVector<int> cells;
            cells.reserve(m_cols);
            for (int col = 0; col < m_cols; ++col) {
                cells.push_back(QRandomGenerator::global()->bounded(kColorCount));  // GCOVR_EXCL_BR_LINE
            }
            candidate.push_back(cells);
        }

        if (GameRules::hasMoves(candidate, m_minimumGroupSize)) return candidate;
        grid = candidate;
    }

    return grid;
}

bool GameBoardController::inBounds(int row, int col) const {
    if (m_gridState.isEmpty()) return false;
    return row >= 0 && row < m_gridState.size() && col >= 0 && col < m_gridState.first().size();
}

void GameBoardController::reset() {
    cancelTimers();
    clearSelections();
    clearPhaseState();
    m_pendingRow = -1;
    m_pendingCol = -1;
    setBusy(false);
    setScore(0);
    setMoves(0);
    setGameOver(false);
    setGridState(buildPlayableGrid());
    setGameOver(m_gridState.isEmpty() || !GameRules::hasMoves(m_gridState, m_minimumGroupSize));
    restartHintIdleTimer();
}

void GameBoardController::removeGroup(int row, int col) {
    if (!inBounds(row, col)) return;
    if (m_gameOver) return;

    registerUserAction();

    if (m_busy) return;
    if (m_gridState[row][col] == GameRules::kEmptyCell) return;

    const auto analysis = GameRules::analyzeSelection(m_gridState, row, col, m_minimumGroupSize);
    if (!analysis.valid) {
        QVariantMap shake;
        shake.insert(QStringLiteral("%1,%2").arg(row).arg(col), true);
        setShakeSet(shake);
        m_shakeClearTimer.start(m_shakeClearMs);
        return;
    }

    setBusy(true);
    m_pendingRow = row;
    m_pendingCol = col;
    setShakeSet({});
    setHintSet({});
    setPopSet(toSelectionMap(analysis.group));
    m_removePreviewTimer.start(m_removePreviewMs);
}

void GameBoardController::registerUserAction() { restartHintIdleTimer(); }

void GameBoardController::setGridForTesting(const GameGrid& grid) {
    cancelTimers();
    clearSelections();
    clearPhaseState();
    m_pendingRow = -1;
    m_pendingCol = -1;
    setBusy(false);
    setScore(0);
    setMoves(0);
    setGameOver(false);
    setGridState(grid);
    setGameOver(m_gridState.isEmpty() || !GameRules::hasMoves(m_gridState, m_minimumGroupSize));
    restartHintIdleTimer();
}

void GameBoardController::setAnimationTimingsForTesting(int removePreviewMs, int removeHoldMs, int fallAnimMs, int shiftAnimMs, int shakeClearMs) {
    m_removePreviewMs = removePreviewMs;
    m_removeHoldMs = removeHoldMs;
    m_fallAnimMs = fallAnimMs;
    m_shiftAnimMs = shiftAnimMs;
    m_shakeClearMs = shakeClearMs;
}

void GameBoardController::setHintTimingsForTesting(int idleHintMs, int hintRepeatMs) {
    m_idleHintDelayMs = idleHintMs;
    m_hintRepeatMs = hintRepeatMs;
}

void GameBoardController::onRemovePreviewTimer() { startResolvedMove(); }

void GameBoardController::onRemoveHoldTimer() { applyFallPhase(); }

void GameBoardController::onFallTimer() {
    setGridState(m_pendingMovePlan.afterFallGrid);
    setMovingTiles({});
    setHiddenSet({});
    applyShiftPhase();
}

void GameBoardController::onShiftTimer() {
    setMovingTiles({});
    setHiddenSet({});
    finishMovePhases();
}

void GameBoardController::onShakeClearTimer() { setShakeSet({}); }

void GameBoardController::onIdleHintTimer() {
    if (triggerHintFlash()) m_hintRepeatTimer.start(m_hintRepeatMs);
}

void GameBoardController::onHintRepeatTimer() {
    if (!triggerHintFlash()) m_hintRepeatTimer.stop();
}
