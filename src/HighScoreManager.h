#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

/**
 * @brief One persisted high-score entry stored internally by the manager.
 *
 * The public @ref HighScoreManager::loadScores API converts @ref when into an ISO-8601 string for QML consumers.
 */
struct HighScoreEntry {
    /// Display name associated with the score.
    QString name;
    /// Score value sorted in descending order.
    int score = 0;
    /// Submission timestamp in milliseconds since the Unix epoch.
    qint64 when = 0;
};

/**
 * @brief Maintains the persisted top-ten scoreboard.
 *
 * Scores are stored as JSON, ordered by descending score and then ascending timestamp, and truncated to the best
 * @c 10 entries.
 */
class HighScoreManager : public QObject {
    Q_OBJECT
    /// Current scoreboard in the same shape as @ref loadScores. Notifies on every change.
    Q_PROPERTY(QVariantList scores READ loadScores NOTIFY scoresChanged)

   public:
    /**
     * @brief Create a manager that stores scores in the default application data location.
     * @param parent Owning QObject parent.
     */
    explicit HighScoreManager(QObject* parent = nullptr);
    /**
     * @brief Create a manager with an explicit storage file path.
     * @param storageFilePath JSON file used for persistence.
     * @param parent Owning QObject parent.
     */
    explicit HighScoreManager(const QString& storageFilePath, QObject* parent = nullptr);

    /**
     * @brief Load the current scoreboard as QML-friendly maps.
     * @return Variant list of maps with keys @c name, @c score, and @c when. Entries are sorted from highest to
     * lowest score, with older timestamps winning ties. The @c when value is returned as an ISO-8601 string.
     */
    Q_INVOKABLE QVariantList loadScores() const;
    /**
     * @brief Check whether a score would enter the top-ten list.
     * @param score Candidate score.
     * @return True when the score is positive and either fewer than ten scores are stored or the score is strictly
     * greater than the current lowest stored score.
     */
    Q_INVOKABLE bool qualifies(int score) const;
    /**
     * @brief Insert one score and persist the updated scoreboard.
     * @param name Player name. The stored value is trimmed and length-limited.
     * @param score Score to record. Non-positive scores are ignored.
     * @return True when the entry was accepted and the scoreboard was written to disk.
     *
     * After insertion the list is sorted by descending score, then by ascending timestamp, truncated to the best ten
     * entries, and written to disk atomically. A failed write keeps the previous file intact.
     */
    Q_INVOKABLE bool submitScore(const QString& name, int score);

   signals:
    /// Emitted whenever the in-memory scoreboard changes.
    void scoresChanged();

   private:
    QString filePath() const;
    void loadFromDisk();
    bool saveToDisk() const;

    QString m_storageFilePath;
    QVector<HighScoreEntry> m_scores;
    static constexpr int kMaxEntries = 10;
};
