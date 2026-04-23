#include "HighScoreManager.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <algorithm>

HighScoreManager::HighScoreManager(QObject* parent) : QObject(parent) { loadFromDisk(); }

QString HighScoreManager::filePath() const {
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/highscores.json";
}

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
}

void HighScoreManager::saveToDisk() const {
    QJsonArray arr;
    for (const auto& e : m_scores) {
        QJsonObject o;
        o.insert("name", e.name);
        o.insert("score", e.score);
        o.insert("when", static_cast<double>(e.when));
        arr.push_back(o);
    }
    QJsonDocument doc(arr);
    QFile f(filePath());
    if (f.open(QIODevice::WriteOnly)) {
        f.write(doc.toJson());
        f.close();
    }
}

QVariantList HighScoreManager::loadScores() {
    QVariantList out;
    for (const auto& e : m_scores) {
        QVariantMap m;
        m["name"] = e.name;
        m["score"] = e.score;
        m["when"] = QDateTime::fromMSecsSinceEpoch(e.when).toString(Qt::ISODate);
        out.push_back(m);
    }
    return out;
}

bool HighScoreManager::qualifies(int score) const {
    if (score <= 0) return false;
    if (m_scores.size() < kMaxEntries) return true;
    int minScore = m_scores.isEmpty() ? -1 : m_scores.last().score;
    return score > minScore;
}

void HighScoreManager::submitScore(const QString& name, int score) {
    if (score <= 0) return;

    HighScoreEntry e{name.trimmed().left(20), score, QDateTime::currentMSecsSinceEpoch()};
    m_scores.push_back(e);
    std::sort(m_scores.begin(), m_scores.end(), [](const auto& a, const auto& b) {
        if (a.score == b.score) return a.when < b.when;
        return a.score > b.score;
    });
    if (m_scores.size() > kMaxEntries) m_scores.resize(kMaxEntries);
    saveToDisk();
}

void HighScoreManager::resetScores() {
    m_scores.clear();
    saveToDisk();
}
