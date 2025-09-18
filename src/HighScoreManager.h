#pragma once
#include <QObject>
#include <QVariantList>
#include <QVector>
#include <QString>
#include <QDateTime>


struct HighScoreEntry {
    QString name;
    int score = 0;
    qint64 when = 0; // msecs since epoch
};


class HighScoreManager : public QObject {
    Q_OBJECT
public:
    explicit HighScoreManager(QObject* parent = nullptr);


    Q_INVOKABLE QVariantList loadScores();
    Q_INVOKABLE bool qualifies(int score) const; // top 10 list
    Q_INVOKABLE void submitScore(const QString &name, int score);
    Q_INVOKABLE void resetScores();


private:
    QString filePath() const;
    void loadFromDisk();
    void saveToDisk() const;


    QVector<HighScoreEntry> m_scores; // sorted desc by score
    static constexpr int kMaxEntries = 10;
};
