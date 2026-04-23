#include <QCoreApplication>
#include <QStandardPaths>
#include <QtTest>

#include "HighScoreManager.h"

class HighScoreManagerTest : public QObject {
    Q_OBJECT

   private slots:
    void initTestCase();
    void init();
    void qualifies_requiresPositiveScore();
    void qualifies_requiresStrictlyBetterThanLowestStoredScore();
    void submitScore_trimsCapsAndPersistsValidEntries();
};

void HighScoreManagerTest::initTestCase() {
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("TapocalypseNowTests");
    QCoreApplication::setApplicationName("HighScoreManagerTests");
}

void HighScoreManagerTest::init() {
    QCoreApplication::setApplicationName(QStringLiteral("HighScoreManagerTests_%1").arg(QString::fromLatin1(QTest::currentTestFunction())));
    HighScoreManager manager;
    manager.resetScores();
}

void HighScoreManagerTest::qualifies_requiresPositiveScore() {
    HighScoreManager manager;

    QVERIFY(!manager.qualifies(0));
    QVERIFY(!manager.qualifies(-5));
    QVERIFY(manager.qualifies(1));
}

void HighScoreManagerTest::qualifies_requiresStrictlyBetterThanLowestStoredScore() {
    HighScoreManager manager;
    for (int index = 1; index <= 10; ++index) {
        manager.submitScore(QStringLiteral("P%1").arg(index), index * 10);
    }

    QVERIFY(!manager.qualifies(10));
    QVERIFY(!manager.qualifies(0));
    QVERIFY(manager.qualifies(11));
}

void HighScoreManagerTest::submitScore_trimsCapsAndPersistsValidEntries() {
    HighScoreManager manager;

    manager.submitScore("   This name is definitely too long   ", 0);
    QCOMPARE(manager.loadScores().size(), 0);

    manager.submitScore("   This name is definitely too long   ", 42);
    const auto scores = manager.loadScores();
    QCOMPARE(scores.size(), 1);

    const auto first = scores.first().toMap();
    QCOMPARE(first.value("score").toInt(), 42);
    QCOMPARE(first.value("name").toString(), QStringLiteral("This name is definit"));
}

QTEST_APPLESS_MAIN(HighScoreManagerTest)

#include "test_highscoremanager.moc"
