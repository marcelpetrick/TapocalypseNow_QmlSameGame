#include <QCoreApplication>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

#include "HighScoreManager.h"

class HighScoreManagerTest : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase();
    void init();
    void defaultStoragePath_isUsedWhenNoCustomPathIsProvided();
    void invalidDiskPayloads_areIgnored();
    void loadFromDisk_sortsAndTruncatesStoredEntries();
    void qualifies_requiresPositiveScore();
    void qualifies_requiresStrictlyBetterThanLowestStoredScore();
    void submitScore_trimsCapsAndPersistsValidEntries();
    void submitScore_sortsTiesAndKeepsTopTen();
    void resetScores_clearsPersistedEntries();
    void directoryPath_doesNotCrashLoadOrSave();
    void submitScore_reportsResultAndNotifies();

   private:
    std::unique_ptr<QTemporaryDir> m_tempDir;

    HighScoreManager createManager() const;
    QString storageFilePath() const;
};

void HighScoreManagerTest::qualifies_requiresPositiveScore() {
    HighScoreManager manager = createManager();

    QVERIFY(!manager.qualifies(0));
    QVERIFY(!manager.qualifies(-5));
    QVERIFY(manager.qualifies(1));
}

void HighScoreManagerTest::defaultStoragePath_isUsedWhenNoCustomPathIsProvided() {
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile::remove(appDataDir + QStringLiteral("/highscores.json"));

    HighScoreManager manager;
    manager.submitScore(QStringLiteral("Ada"), 77);

    QFile persisted(appDataDir + QStringLiteral("/highscores.json"));
    QVERIFY(persisted.exists());
}

void HighScoreManagerTest::invalidDiskPayloads_areIgnored() {
    {
        QFile invalidJson(storageFilePath());
        QVERIFY(invalidJson.open(QIODevice::WriteOnly));
        invalidJson.write("not-json");
    }
    QCOMPARE(createManager().loadScores().size(), 0);

    {
        QFile objectJson(storageFilePath());
        QVERIFY(objectJson.open(QIODevice::WriteOnly | QIODevice::Truncate));
        objectJson.write("{\"name\":\"Ada\"}");
    }
    QCOMPARE(createManager().loadScores().size(), 0);
}

void HighScoreManagerTest::loadFromDisk_sortsAndTruncatesStoredEntries() {
    QFile file(storageFilePath());
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"([
        {"name":"Low","score":10,"when":1000},
        {"name":"HighLater","score":200,"when":3000},
        {"name":"HighEarlier","score":200,"when":2000},
        {"name":"Mid","score":100,"when":1500},
        {"name":"P5","score":50,"when":5},
        {"name":"P6","score":60,"when":6},
        {"name":"P7","score":70,"when":7},
        {"name":"P8","score":80,"when":8},
        {"name":"P9","score":90,"when":9},
        {"name":"P10","score":95,"when":10},
        {"name":"P11","score":96,"when":11},
        {"name":"P12","score":97,"when":12}
    ])");
    file.close();

    HighScoreManager manager = createManager();
    const auto scores = manager.loadScores();
    QCOMPARE(scores.size(), 10);
    QCOMPARE(scores.first().toMap().value("name").toString(), QStringLiteral("HighEarlier"));
    QCOMPARE(scores[1].toMap().value("name").toString(), QStringLiteral("HighLater"));
    QCOMPARE(scores.last().toMap().value("score").toInt(), 60);
}

void HighScoreManagerTest::qualifies_requiresStrictlyBetterThanLowestStoredScore() {
    HighScoreManager manager = createManager();
    for (int index = 1; index <= 10; ++index) {
        manager.submitScore(QStringLiteral("P%1").arg(index), index * 10);
    }

    QVERIFY(!manager.qualifies(10));
    QVERIFY(!manager.qualifies(0));
    QVERIFY(manager.qualifies(11));
}

void HighScoreManagerTest::init() { QFile::remove(storageFilePath()); }

void HighScoreManagerTest::submitScore_trimsCapsAndPersistsValidEntries() {
    HighScoreManager manager = createManager();

    manager.submitScore("   This name is definitely too long   ", 0);
    QCOMPARE(manager.loadScores().size(), 0);

    manager.submitScore("   This name is definitely too long   ", 42);
    const auto scores = manager.loadScores();
    QCOMPARE(scores.size(), 1);

    const auto first = scores.first().toMap();
    QCOMPARE(first.value("score").toInt(), 42);
    QCOMPARE(first.value("name").toString(), QStringLiteral("This name is definit"));
}

void HighScoreManagerTest::submitScore_sortsTiesAndKeepsTopTen() {
    HighScoreManager manager = createManager();
    for (int index = 0; index < 12; ++index) {
        manager.submitScore(QStringLiteral("P%1").arg(index), index < 2 ? 100 : index);
        QTest::qWait(1);
    }

    const auto scores = manager.loadScores();
    QCOMPARE(scores.size(), 10);
    QCOMPARE(scores[0].toMap().value("score").toInt(), 100);
    QCOMPARE(scores[1].toMap().value("score").toInt(), 100);
    QVERIFY(scores[0].toMap().value("when").toString() <= scores[1].toMap().value("when").toString());
    QCOMPARE(scores.last().toMap().value("score").toInt(), 4);
}

void HighScoreManagerTest::resetScores_clearsPersistedEntries() {
    HighScoreManager manager = createManager();
    manager.submitScore(QStringLiteral("Ada"), 42);
    QVERIFY(!manager.loadScores().isEmpty());

    manager.resetScores();

    QCOMPARE(manager.loadScores().size(), 0);
    HighScoreManager reloaded = createManager();
    QCOMPARE(reloaded.loadScores().size(), 0);
}

void HighScoreManagerTest::directoryPath_doesNotCrashLoadOrSave() {
    QDir dir(m_tempDir->path() + QStringLiteral("/as-directory"));
    QVERIFY(QDir().mkpath(dir.path()));

    HighScoreManager manager(dir.path());
    QCOMPARE(manager.loadScores().size(), 0);
    QVERIFY(!manager.submitScore(QStringLiteral("Ada"), 42));
    QCOMPARE(manager.loadScores().size(), 1);
    QVERIFY(dir.exists());
}

void HighScoreManagerTest::submitScore_reportsResultAndNotifies() {
    HighScoreManager manager = createManager();
    QSignalSpy changed(&manager, &HighScoreManager::scoresChanged);

    QVERIFY(!manager.submitScore(QStringLiteral("Zero"), 0));
    QCOMPARE(changed.count(), 0);

    QVERIFY(manager.submitScore(QStringLiteral("Ada"), 42));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(manager.property("scores").toList().size(), 1);

    manager.resetScores();
    QCOMPARE(changed.count(), 2);
    QVERIFY(manager.property("scores").toList().isEmpty());
}

HighScoreManager HighScoreManagerTest::createManager() const { return HighScoreManager(storageFilePath()); }

QString HighScoreManagerTest::storageFilePath() const { return m_tempDir->path() + QStringLiteral("/highscores.json"); }

void HighScoreManagerTest::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("TapocalypseNowTests");
    QCoreApplication::setApplicationName("HighScoreManagerTests");
    m_tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_tempDir->isValid());
}

QTEST_APPLESS_MAIN(HighScoreManagerTest)

#include "test_highscoremanager.moc"
