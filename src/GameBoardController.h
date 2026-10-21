#pragma once

#include <QtQml/qqmlregistration.h>

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "GameRules.h"

/**
 * @brief Stateful controller that orchestrates board state, animation phases, and idle hints.
 *
 * The controller owns the authoritative board state exposed to QML. Rendering remains in QML,
 * while move resolution, hint timing, and animation staging stay testable here.
 *
 * QML-facing board data uses nested row-major @c QVariantList values with integer cell contents. Empty cells use
 * @ref GameRules::kEmptyCell. Selection maps use keys in the form @c "row,col" with boolean @c true values. Motion
 * lists contain maps with @c color, @c fromRow, @c fromCol, @c toRow, @c toCol, and @c duration fields.
 */
class GameBoardControllerTest;

class GameBoardController : public QObject {
    Q_OBJECT
    QML_ELEMENT

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
    Q_PROPERTY(QVariantMap hoverSet READ hoverSet NOTIFY hoverSetChanged)
    Q_PROPERTY(int hintPulseEpoch READ hintPulseEpoch NOTIFY hintPulseEpochChanged)

   public:
    /**
     * @brief Create a board controller with default dimensions and timings.
     * @param parent Owning QObject parent.
     */
    explicit GameBoardController(QObject* parent = nullptr);

    /// @return Current board row count.
    int rows() const;
    /**
     * @brief Set the board row count used by the next reset.
     * @param rows New row count.
     */
    void setRows(int rows);

    /// @return Current board column count.
    int cols() const;
    /**
     * @brief Set the board column count used by the next reset.
     * @param cols New column count.
     */
    void setCols(int cols);

    /// @return Minimum connected group size required for a valid move.
    int minimumGroupSize() const;
    /**
     * @brief Set the minimum connected group size required for a valid move.
     * @param minimumGroupSize New move threshold.
     */
    void setMinimumGroupSize(int minimumGroupSize);

    /// @return Current accumulated score.
    int score() const;
    /// @return Number of successful moves performed in the current game.
    int moves() const;
    /// @return True when the current board has no valid moves left.
    bool gameOver() const;
    /// @return True while an animation or staged move resolution is active.
    bool busy() const;

    /// @return Board cells as a nested row-major QVariantList for QML consumption. Empty cells use @ref GameRules::kEmptyCell.
    QVariantList grid() const;
    /// @return Lookup map for tiles currently in the pop-preview phase, keyed by @c "row,col".
    QVariantMap popSet() const;
    /// @return Lookup map for tiles currently shaking after an invalid selection, keyed by @c "row,col".
    QVariantMap shakeSet() const;
    /// @return Lookup map for tiles hidden because an animated proxy tile is in flight, keyed by @c "row,col".
    QVariantMap hiddenSet() const;
    /// @return Animated tile descriptors for active fall or shift phases. Items contain @c color, @c fromRow,
    /// @c fromCol, @c toRow, @c toCol, and @c duration.
    QVariantList movingTiles() const;
    /// @return Lookup map for tiles highlighted by the idle hint cycle, keyed by @c "row,col".
    QVariantMap hintSet() const;
    /// @return Lookup map for the removable group under the pointer, keyed by @c "row,col"; empty when none.
    QVariantMap hoverSet() const;
    /// @return Monotonic pulse value incremented whenever a new idle hint flash is triggered.
    int hintPulseEpoch() const;

    /**
     * @brief Access the authoritative native board state.
     * @return Current board grid.
     */
    const GameGrid& gridState() const;

    /**
     * @brief Start a new game using the configured dimensions.
     *
     * Resets score, moves, pending animation state, and hint state before creating a fresh playable board. If no
     * playable board can be built, the controller ends in game-over state.
     */
    Q_INVOKABLE void reset();
    /**
     * @brief Attempt to remove the group at one board position.
     * @param row Zero-based row index.
     * @param col Zero-based column index.
     *
     * Out-of-bounds clicks, clicks on empty cells, clicks during game over, and clicks while a staged move is already
     * running are ignored. Invalid groups trigger shake feedback for the clicked tile. Valid groups start the
     * asynchronous staged move resolution managed by this controller.
     */
    Q_INVOKABLE void removeGroup(int row, int col);
    /**
     * @brief Notify the controller about a user interaction that should reset idle hints.
     *
     * This restarts the idle-hint delay timer so the hint cycle waits for the next inactivity period.
     */
    Q_INVOKABLE void registerUserAction();
    /**
     * @brief End the current run early and hand its score to the highscore flow.
     *
     * Ignored before the first move, while a move is still resolving, and after the game is already over. Otherwise the board enters game-over
     * state and @ref gameEnded is emitted with the current score, exactly as if no move were left.
     */
    Q_INVOKABLE void endRun();
    /**
     * @brief Preview the group under the pointer.
     * @param row Zero-based row index, or a negative value when the pointer left the board.
     * @param col Zero-based column index.
     *
     * @ref hoverSet holds the group at this cell when it is removable and the board accepts moves, and is empty
     * otherwise. The preview is re-evaluated after each resolved move, since the board under the pointer changes.
     */
    Q_INVOKABLE void setHoverCell(int row, int col);
    /**
     * @brief Clear the preview when the pointer leaves a cell, unless another cell already took over.
     * @param row Zero-based row index of the cell that was left.
     * @param col Zero-based column index of the cell that was left.
     */
    Q_INVOKABLE void leaveHoverCell(int row, int col);
    /**
     * @brief Execute one deterministic automated benchmark action.
     *
     * Selects the first valid group in scan order. A finished game is reset,
     * while calls made during an active animation are ignored.
     */
    Q_INVOKABLE void benchmarkStep();

    /**
     * @brief Replace the current board state directly for tests.
     * @param grid Grid to install as the active board.
     *
     * Test support API. This bypasses normal random board creation and resets score, move, timer, and phase state.
     */
    void setGridForTesting(const GameGrid& grid);
    /**
     * @brief Override animation timings for tests.
     * @param removePreviewMs Duration of the pop preview phase.
     * @param removeHoldMs Duration of the hidden/removed hold phase.
     * @param fallAnimMs Duration of the fall animation phase.
     * @param shiftAnimMs Duration of the column shift animation phase.
     * @param shakeClearMs Delay before invalid-selection shake markers are cleared.
     *
     * Test support API.
     */
    void setAnimationTimingsForTesting(int removePreviewMs, int removeHoldMs, int fallAnimMs, int shiftAnimMs, int shakeClearMs);
    /**
     * @brief Override idle hint timings for tests.
     * @param idleHintMs Delay before the first idle hint flash starts.
     * @param hintRepeatMs Delay between repeated hint flashes.
     *
     * Test support API.
     */
    void setHintTimingsForTesting(int idleHintMs, int hintRepeatMs);

   signals:
    /// Emitted when the configured row count changes.
    void rowsChanged();
    /// Emitted when the configured column count changes.
    void colsChanged();
    /// Emitted when the minimum valid group size changes.
    void minimumGroupSizeChanged();
    /// Emitted when the score changes.
    void scoreChanged();
    /// Emitted when the move count changes.
    void movesChanged();
    /// Emitted when the game-over state changes.
    void gameOverChanged();
    /// Emitted when the busy state changes.
    void busyChanged();
    /// Emitted when the exposed grid changes.
    void gridChanged();
    /// Emitted when the pop-preview selection changes.
    void popSetChanged();
    /// Emitted when the invalid-selection shake markers change.
    void shakeSetChanged();
    /// Emitted when the hidden tile map changes.
    void hiddenSetChanged();
    /// Emitted when animated tile descriptors change.
    void movingTilesChanged();
    /// Emitted when the idle hint selection changes.
    void hintSetChanged();
    /// Emitted when the hover preview changes.
    void hoverSetChanged();
    /// Emitted when the idle hint pulse epoch increments.
    void hintPulseEpochChanged();
    /**
     * @brief Emitted when the board reaches a terminal no-moves state.
     * @param score Final score for the completed game.
     *
     * This is emitted after a resolved move leaves no remaining legal move on the board.
     */
    void gameEnded(int score);

   private:
    // White-box tests drive the internal move phases directly.
    friend class ::GameBoardControllerTest;

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
    void updateHoverSet();

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

   private slots:
    void onRemovePreviewTimer();
    void onRemoveHoldTimer();
    void onFallTimer();
    void onShiftTimer();
    void onShakeClearTimer();
    void onIdleHintTimer();
    void onHintRepeatTimer();

   private:
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
    QVariantMap m_hoverSet;
    int m_hoverRow = -1;
    int m_hoverCol = -1;
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
