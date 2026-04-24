#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "GameRules.h"

class GameBoardController : public QObject {
    Q_OBJECT

    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY rowsChanged)
    Q_PROPERTY(int cols READ cols WRITE setCols NOTIFY colsChanged)
    Q_PROPERTY(int minimumGroupSize READ minimumGroupSize WRITE setMinimumGroupSize NOTIFY minimumGroupSizeChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int moves READ moves NOTIFY movesChanged)
    Q_PROPERTY(bool gameOver READ gameOver NOTIFY gameOverChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QVariantList grid READ grid NOTIFY gridChanged)
    Q_PROPERTY(QVariantMap popSet READ popSet NOTIFY popSetChanged)
    Q_PROPERTY(QVariantMap shakeSet READ shakeSet NOTIFY shakeSetChanged)
    Q_PROPERTY(QVariantMap hiddenSet READ hiddenSet NOTIFY hiddenSetChanged)
    Q_PROPERTY(QVariantList movingTiles READ movingTiles NOTIFY movingTilesChanged)
    Q_PROPERTY(QVariantMap hintSet READ hintSet NOTIFY hintSetChanged)
    Q_PROPERTY(int hintPulseEpoch READ hintPulseEpoch NOTIFY hintPulseEpochChanged)

   public:
    explicit GameBoardController(QObject* parent = nullptr);

    int rows() const;
    void setRows(int rows);

    int cols() const;
    void setCols(int cols);

    int minimumGroupSize() const;
    void setMinimumGroupSize(int minimumGroupSize);

    int score() const;
    int moves() const;
    bool gameOver() const;
    bool busy() const;

    QVariantList grid() const;
    QVariantMap popSet() const;
    QVariantMap shakeSet() const;
    QVariantMap hiddenSet() const;
    QVariantList movingTiles() const;
    QVariantMap hintSet() const;
    int hintPulseEpoch() const;

    const GameGrid& gridState() const;

    Q_INVOKABLE void reset();
    Q_INVOKABLE void removeGroup(int row, int col);
    Q_INVOKABLE void registerUserAction();

    void setGridForTesting(const GameGrid& grid);
    void setAnimationTimingsForTesting(int removePreviewMs, int removeHoldMs, int fallAnimMs, int shiftAnimMs, int shakeClearMs);
    void setHintTimingsForTesting(int idleHintMs, int hintRepeatMs);

   signals:
    void rowsChanged();
    void colsChanged();
    void minimumGroupSizeChanged();
    void scoreChanged();
    void movesChanged();
    void gameOverChanged();
    void busyChanged();
    void gridChanged();
    void popSetChanged();
    void shakeSetChanged();
    void hiddenSetChanged();
    void movingTilesChanged();
    void hintSetChanged();
    void hintPulseEpochChanged();
    void gameEnded(int score);

   private:
    static constexpr int kColorCount = 6;
    static constexpr int kDefaultRemovePreviewMs = 220;
    static constexpr int kDefaultRemoveHoldMs = 200;
    static constexpr int kDefaultFallAnimMs = 320;
    static constexpr int kDefaultShiftAnimMs = 260;
    static constexpr int kDefaultShakeClearMs = 180;
    static constexpr int kDefaultIdleHintDelayMs = 3000;
    static constexpr int kDefaultHintRepeatMs = 2000;
    static constexpr int kMaxResetAttempts = 128;

    void setScore(int score);
    void setMoves(int moves);
    void setGameOver(bool gameOver);
    void setBusy(bool busy);

    void setGridState(const GameGrid& grid);
    void setPopSet(const QVariantMap& popSet);
    void setShakeSet(const QVariantMap& shakeSet);
    void setHiddenSet(const QVariantMap& hiddenSet);
    void setMovingTiles(const QVariantList& movingTiles);
    void setHintSet(const QVariantMap& hintSet);
    void incrementHintPulseEpoch();

    void clearSelections();
    void clearPhaseState();
    void cancelTimers();
    void stopHintCycle();
    void restartHintIdleTimer();
    bool triggerHintFlash();
    void hideMotionSources(const QVector<TileMotion>& motions);
    void buildAnimatedTiles(const QVector<TileMotion>& motions, int durationMs);
    void startResolvedMove();
    void applyFallPhase();
    void applyShiftPhase();
    void finishMovePhases();
    GameGrid buildPlayableGrid() const;
    bool inBounds(int row, int col) const;

    void onRemovePreviewTimer();
    void onRemoveHoldTimer();
    void onFallTimer();
    void onShiftTimer();
    void onShakeClearTimer();
    void onIdleHintTimer();
    void onHintRepeatTimer();

    int m_rows = 10;
    int m_cols = 16;
    int m_minimumGroupSize = 3;
    int m_score = 0;
    int m_moves = 0;
    bool m_gameOver = false;
    bool m_busy = false;

    int m_removePreviewMs = kDefaultRemovePreviewMs;
    int m_removeHoldMs = kDefaultRemoveHoldMs;
    int m_fallAnimMs = kDefaultFallAnimMs;
    int m_shiftAnimMs = kDefaultShiftAnimMs;
    int m_shakeClearMs = kDefaultShakeClearMs;
    int m_idleHintDelayMs = kDefaultIdleHintDelayMs;
    int m_hintRepeatMs = kDefaultHintRepeatMs;

    GameGrid m_gridState;
    QVariantList m_grid;
    QVariantMap m_popSet;
    QVariantMap m_shakeSet;
    QVariantMap m_hiddenSet;
    QVariantList m_movingTiles;
    QVariantMap m_hintSet;
    int m_hintPulseEpoch = 0;

    int m_pendingRow = -1;
    int m_pendingCol = -1;
    bool m_phaseHasMoves = false;
    MovePlan m_pendingMovePlan;

    QTimer m_removePreviewTimer;
    QTimer m_removeHoldTimer;
    QTimer m_fallTimer;
    QTimer m_shiftTimer;
    QTimer m_shakeClearTimer;
    QTimer m_idleHintTimer;
    QTimer m_hintRepeatTimer;
};
