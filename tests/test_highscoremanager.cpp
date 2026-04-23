#include <QCoreApplication>
#include <QFile>
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
    void qualifies_requiresPositiveScore();
    void qualifies_requiresStrictlyBetterThanLowestStoredScore();
    void submitScore_trimsCapsAndPersistsValidEntries();

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
