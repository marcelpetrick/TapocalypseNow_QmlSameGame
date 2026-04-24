#pragma once
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

/**
 * @brief One persisted high-score entry.
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
 */
class HighScoreManager : public QObject {
    Q_OBJECT
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
     * @return Variant list of high-score entries sorted from highest to lowest score.
     */
    Q_INVOKABLE QVariantList loadScores();
    /**
     * @brief Check whether a score would enter the top-ten list.
     * @param score Candidate score.
     * @return True when the score is positive and ranks above the current cutoff.
     */
    Q_INVOKABLE bool qualifies(int score) const;
    /**
     * @brief Insert one score and persist the updated scoreboard.
     * @param name Player name. The stored value is trimmed and length-limited.
     * @param score Score to record. Non-positive scores are ignored.
     */
    Q_INVOKABLE void submitScore(const QString& name, int score);
    /**
     * @brief Remove every stored score and persist the empty list.
     */
    Q_INVOKABLE void resetScores();

   private:
    QString filePath() const;
    void loadFromDisk();
    void saveToDisk() const;

    QString m_storageFilePath;
    QVector<HighScoreEntry> m_scores;
    static constexpr int kMaxEntries = 10;
};
