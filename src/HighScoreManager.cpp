#include "HighScoreManager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <algorithm>

HighScoreManager::HighScoreManager(QObject* parent) : QObject(parent) { loadFromDisk(); }

HighScoreManager::HighScoreManager(const QString& storageFilePath, QObject* parent) : QObject(parent), m_storageFilePath(storageFilePath) { loadFromDisk(); }

QString HighScoreManager::filePath() const {
    if (!m_storageFilePath.isEmpty()) {
        const QFileInfo fileInfo(m_storageFilePath);
        QDir().mkpath(fileInfo.absolutePath());
        return m_storageFilePath;
    }

    const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/highscores.json";
}  // GCOVR_EXCL_LINE

void HighScoreManager::loadFromDisk() {
    m_scores.clear();
    QFile f(filePath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isArray()) return;
    for (const auto& v : doc.array()) {
        const auto o = v.toObject();
        HighScoreEntry e;
        e.name = o.value("name").toString();
        e.score = o.value("score").toInt();
        e.when = static_cast<qint64>(o.value("when").toDouble());
        m_scores.push_back(e);
    }
    std::sort(m_scores.begin(), m_scores.end(), [](const auto& a, const auto& b) {
        if (a.score == b.score) return a.when < b.when;  // older first
        return a.score > b.score;
    });
    if (m_scores.size() > kMaxEntries) m_scores.resize(kMaxEntries);
}  // GCOVR_EXCL_LINE

bool HighScoreManager::saveToDisk() const {
    QJsonArray arr;
    for (const auto& e : m_scores) {
        QJsonObject o;
        o.insert("name", e.name);
        o.insert("score", e.score);
        o.insert("when", static_cast<double>(e.when));
        arr.push_back(o);
    }
    // QSaveFile writes to a temporary file and renames it on commit, so a crash or a full disk never truncates the
    // existing scoreboard.
    QSaveFile f(filePath());
    if (!f.open(QIODevice::WriteOnly) || f.write(QJsonDocument(arr).toJson()) < 0 || !f.commit()) {
        qWarning().noquote() << "Tapocalypse: could not save highscores to" << f.fileName() << "-" << f.errorString();
        return false;
    }
    return true;
}

QVariantList HighScoreManager::loadScores() const {
    QVariantList out;
    for (const auto& e : m_scores) {
        QVariantMap m;
        m["name"] = e.name;
        m["score"] = e.score;
        m["when"] = QDateTime::fromMSecsSinceEpoch(e.when).toString(Qt::ISODate);
        out.push_back(m);
    }
    return out;  // GCOVR_EXCL_LINE
}  // GCOVR_EXCL_LINE

bool HighScoreManager::qualifies(int score) const {
    if (score <= 0) return false;
    if (m_scores.size() < kMaxEntries) return true;
    const int minScore = m_scores.last().score;
    return score > minScore;
}

bool HighScoreManager::submitScore(const QString& name, int score) {
    if (score <= 0) return false;

    HighScoreEntry e{name.trimmed().left(20), score, QDateTime::currentMSecsSinceEpoch()};  // GCOVR_EXCL_BR_LINE
    m_scores.push_back(e);
    std::sort(m_scores.begin(), m_scores.end(), [](const auto& a, const auto& b) {
        if (a.score == b.score) return a.when < b.when;
        return a.score > b.score;
    });
    if (m_scores.size() > kMaxEntries) m_scores.resize(kMaxEntries);
    emit scoresChanged();
    return saveToDisk();
}

void HighScoreManager::resetScores() {
    m_scores.clear();
    emit scoresChanged();
    saveToDisk();
}
