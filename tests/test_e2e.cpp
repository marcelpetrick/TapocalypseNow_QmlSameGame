/**
 * End-to-end tests: load the shipped main.qml offscreen and play it through real mouse and key events.
 *
 * Set TAPOCALYPSE_E2E_SCREENSHOT_DIR to save a screenshot per checked window size.
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExtensionPlugin>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <memory>

#include "GameBoardController.h"
#include "GameRules.h"
#include "HighScoreManager.h"
#include "SoundEffects.h"

Q_IMPORT_QML_PLUGIN(TapocalypsePlugin)

namespace {

GameGrid makeGrid(std::initializer_list<std::initializer_list<int>> rows) {
    GameGrid grid;
    for (const auto& rowValues : rows) {
        grid.push_back(QVector<int>(rowValues));
    }
    return grid;
}

int tileCount(const GameGrid& grid) {
    int count = 0;
    for (const auto& row : grid) {
        for (const auto cell : row) {
            if (cell != GameRules::kEmptyCell) ++count;
        }
    }
    return count;
}

}  // namespace

class EndToEndTest : public QObject {
    Q_OBJECT

   private slots:
    void init();
    void cleanup();
    void legalClick_removesWholeGroupAndScoresSquare();
    void illegalClick_changesNothing();
    void playingToGameOver_offersHighscoreEntryOnce();
    void endRun_submitsPartialRun();
    void sounds_followTheGame();
    void soundButton_mutesAndRemembers();
    void layout_fitsEveryScreenShape_data();
    void layout_fitsEveryScreenShape();

   private:
    QQuickWindow* window() const;
    QQuickItem* item(const char* objectName) const;
    GameBoardController* controller() const;
    QObject* scoreDialog() const;
    QPoint tileCenter(int row, int col) const;
    void click(QQuickItem* target) const;
    void clickTile(int row, int col) const;
    void waitUntilIdle() const;
    QStringList takeCues();

    std::unique_ptr<QTemporaryDir> m_dir;
    std::unique_ptr<HighScoreManager> m_scores;
    std::unique_ptr<SoundEffects> m_sounds;
    std::unique_ptr<QSignalSpy> m_cues;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
};

void EndToEndTest::init() {
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    m_scores = std::make_unique<HighScoreManager>(m_dir->filePath(QStringLiteral("highscores.json")));
    // Real cue player, inaudible: the tests listen to cuePlayed instead of the speakers.
    m_sounds = std::make_unique<SoundEffects>(m_dir->filePath(QStringLiteral("settings.ini")));
    m_sounds->setVolume(0);
    m_cues = std::make_unique<QSignalSpy>(m_sounds.get(), &SoundEffects::cuePlayed);
    m_engine = std::make_unique<QQmlApplicationEngine>();
    m_engine->rootContext()->setContextProperty(QStringLiteral("highScoreManager"), m_scores.get());
    m_engine->rootContext()->setContextProperty(QStringLiteral("soundEffects"), m_sounds.get());
    m_engine->load(QUrl(QStringLiteral("qrc:/Tapocalypse/qml/main.qml")));
    QVERIFY(window());
    window()->resize(1280, 800);
    QVERIFY(QTest::qWaitForWindowExposed(window()));

    // Wait for the deferred initial deal, then make the run deterministic and fast.
    QTRY_VERIFY(!controller()->gridState().isEmpty());
    controller()->setAnimationTimingsForTesting(1, 1, 1, 1, 1);
    controller()->setHintTimingsForTesting(100000, 100000);
}

void EndToEndTest::cleanup() {
    m_engine.reset();
    m_cues.reset();
    m_sounds.reset();
    m_scores.reset();
    m_dir.reset();
}

QQuickWindow* EndToEndTest::window() const {
    const auto roots = m_engine->rootObjects();
    return roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(roots.constFirst());
}

QQuickItem* EndToEndTest::item(const char* objectName) const { return window()->findChild<QQuickItem*>(QLatin1String(objectName)); }

GameBoardController* EndToEndTest::controller() const { return window()->findChild<GameBoardController*>(QStringLiteral("controller")); }

QObject* EndToEndTest::scoreDialog() const { return window()->findChild<QObject*>(QStringLiteral("scoreDialog")); }

QPoint EndToEndTest::tileCenter(int row, int col) const {
    const auto* board = item("gameBoard");
    const auto* tiles = item("tileView");
    const qreal cell = board->property("cell").toReal();
    const qreal pitch = board->property("pitch").toReal();
    return tiles->mapToScene(QPointF(col * pitch + cell / 2, row * pitch + cell / 2)).toPoint();
}

void EndToEndTest::click(QQuickItem* target) const {
    QVERIFY(target);
    const QPoint center = target->mapToScene(QPointF(target->width() / 2, target->height() / 2)).toPoint();
    QTest::mouseClick(window(), Qt::LeftButton, Qt::NoModifier, center);
}

void EndToEndTest::clickTile(int row, int col) const { QTest::mouseClick(window(), Qt::LeftButton, Qt::NoModifier, tileCenter(row, col)); }

void EndToEndTest::waitUntilIdle() const { QTRY_VERIFY_WITH_TIMEOUT(!controller()->busy(), 2000); }

QStringList EndToEndTest::takeCues() {
    QStringList names;
    for (const auto& call : *m_cues) names << call.constFirst().toString();
    m_cues->clear();
    return names;
}

void EndToEndTest::legalClick_removesWholeGroupAndScoresSquare() {
    // RULES.md §5–§8: the whole orthogonal group goes, score += N*N, tiles fall, empty columns shift left.
    controller()->setGridForTesting(makeGrid({
        {0, 1, 2, 3},
        {0, 1, 2, 3},
        {0, 0, 4, 5},
    }));

    // Hovering previews exactly the group the click will remove.
    QTest::mouseMove(window(), tileCenter(2, 1));
    QTRY_COMPARE(controller()->hoverSet().size(), 4);
    QVERIFY(controller()->hoverSet().contains(QStringLiteral("0,0")));
    const QString shotDir = qEnvironmentVariable("TAPOCALYPSE_E2E_SCREENSHOT_DIR");
    if (!shotDir.isEmpty()) {
        QTest::qWait(300);
        window()->grabWindow().save(shotDir + QStringLiteral("/hover-preview.png"));
    }

    clickTile(2, 1);
    waitUntilIdle();

    QCOMPARE(controller()->score(), 16);
    QCOMPARE(controller()->moves(), 1);
    QCOMPARE(controller()->gridState(), makeGrid({
                                            {-1, 2, 3, -1},
                                            {1, 2, 3, -1},
                                            {1, 4, 5, -1},
                                        }));
}

void EndToEndTest::illegalClick_changesNothing() {
    // RULES.md §5: groups of one or two are not removable and leave score, moves and board untouched.
    const GameGrid grid = makeGrid({
        {0, 0, 1},
        {2, 3, 1},
        {2, 2, 2},
    });
    controller()->setGridForTesting(grid);

    clickTile(0, 0);
    clickTile(1, 1);
    waitUntilIdle();

    QCOMPARE(controller()->score(), 0);
    QCOMPARE(controller()->moves(), 0);
    QCOMPARE(controller()->gridState(), grid);
}

void EndToEndTest::playingToGameOver_offersHighscoreEntryOnce() {
    QSignalSpy ended(controller(), &GameBoardController::gameEnded);
    controller()->setGridForTesting(makeGrid({
        {0, 1, 1, 1},
        {0, 2, 2, 2},
        {0, 3, 4, 5},
    }));

    // Play the first legal group in scan order until the board is stuck, clicking like a player would.
    int expectedScore = 0;
    while (!controller()->gameOver()) {
        const auto pick = GameRules::findFirstValidSelection(controller()->gridState(), 3);
        QVERIFY(pick.valid);
        const int before = tileCount(controller()->gridState());
        clickTile(pick.group.first().row, pick.group.first().col);
        waitUntilIdle();
        expectedScore += pick.groupSize * pick.groupSize;
        QCOMPARE(tileCount(controller()->gridState()), before - pick.groupSize);
    }

    QCOMPARE(ended.count(), 1);
    QCOMPARE(controller()->score(), expectedScore);
    QVERIFY(!GameRules::hasMoves(controller()->gridState(), 3));

    // RULES.md §11: no gameplay mutation after game over.
    const GameGrid frozen = controller()->gridState();
    clickTile(2, 0);
    waitUntilIdle();
    QCOMPARE(controller()->gridState(), frozen);

    // The dialog asks for a name; Enter submits exactly once.
    QTRY_VERIFY(scoreDialog()->property("opened").toBool());
    QVERIFY(!scoreDialog()->property("browsing").toBool());
    for (const char key : QByteArrayLiteral("  Ada  ")) QTest::keyClick(window(), key);
    QTest::keyClick(window(), Qt::Key_Return);
    QTest::keyClick(window(), Qt::Key_Return);

    const auto scores = m_scores->loadScores();
    QCOMPARE(scores.size(), 1);
    QCOMPARE(scores.first().toMap().value("name").toString(), QStringLiteral("Ada"));
    QCOMPARE(scores.first().toMap().value("score").toInt(), expectedScore);
    QVERIFY(scoreDialog()->property("browsing").toBool());
    QCOMPARE(item("scoresView")->property("count").toInt(), 1);
}

void EndToEndTest::endRun_submitsPartialRun() {
    controller()->setGridForTesting(makeGrid({
        {0, 0, 0, 1},
        {2, 2, 2, 1},
        {3, 3, 3, 1},
    }));
    QVERIFY(!item("endRunButton")->isEnabled());

    clickTile(0, 0);
    waitUntilIdle();
    QVERIFY(!controller()->gameOver());
    QVERIFY(item("endRunButton")->isEnabled());

    click(item("endRunButton"));
    QVERIFY(controller()->gameOver());
    QTRY_VERIFY(scoreDialog()->property("opened").toBool());
    QCOMPARE(scoreDialog()->property("candidateScore").toInt(), 9);
}

void EndToEndTest::sounds_followTheGame() {
    // The initial deal plays the arpeggio.
    QCOMPARE(takeCues(), QStringList({"deal"}));

    controller()->setGridForTesting(makeGrid({
        {0, 0, 1, 2},
        {0, 0, 1, 2},
        {0, 0, 3, 3},
    }));
    // Too small: the "nuh-uh".
    clickTile(2, 2);
    waitUntilIdle();
    QCOMPARE(takeCues(), QStringList({"reject"}));

    // Six tiles pop with the second tier, then the column slide settles with a thud and the stuck board ends the run.
    clickTile(0, 0);
    waitUntilIdle();
    QCOMPARE(takeCues(), QStringList({"pop2", "land", "gameover"}));

    // A saved highscore gets the fanfare.
    QTRY_VERIFY(scoreDialog()->property("opened").toBool());
    for (const char key : QByteArrayLiteral("Bo")) QTest::keyClick(window(), key);
    QTest::keyClick(window(), Qt::Key_Return);
    QCOMPARE(takeCues(), QStringList({"highscore"}));

    QMetaObject::invokeMethod(scoreDialog(), "close");
    click(item("newGameButton"));
    QCOMPARE(takeCues(), QStringList({"deal"}));
}

void EndToEndTest::soundButton_mutesAndRemembers() {
    auto* button = item("soundButton");
    QVERIFY(button);
    QCOMPARE(button->isVisible(), m_sounds->available());
    if (!m_sounds->available()) QSKIP("built without Qt Multimedia: the sound button stays hidden");

    QCOMPARE(button->property("text").toString(), QStringLiteral("Sound: On"));
    click(button);
    QVERIFY(m_sounds->muted());
    QCOMPARE(button->property("text").toString(), QStringLiteral("Sound: Off"));
    takeCues();
    click(item("newGameButton"));
    QCOMPARE(takeCues(), QStringList());
    QVERIFY(SoundEffects(m_dir->filePath(QStringLiteral("settings.ini"))).muted());

    // M toggles it back from the keyboard.
    QTest::keyClick(window(), Qt::Key_M);
    QVERIFY(!m_sounds->muted());
    QCOMPARE(button->property("text").toString(), QStringLiteral("Sound: On"));
}

void EndToEndTest::layout_fitsEveryScreenShape_data() {
    QTest::addColumn<QSize>("size");
    QTest::newRow("minimum") << QSize(480, 360);
    QTest::newRow("laptop") << QSize(1366, 768);
    QTest::newRow("full-hd") << QSize(1920, 1080);
    QTest::newRow("portrait") << QSize(600, 1000);
    QTest::newRow("ultrawide") << QSize(2560, 720);
}

void EndToEndTest::layout_fitsEveryScreenShape() {
    QFETCH(QSize, size);
    window()->resize(size);
    QTRY_COMPARE(window()->size(), size);
    QCoreApplication::processEvents();

    const QRectF frame = item("boardFrame")->mapRectToScene(item("boardFrame")->boundingRect());
    const QRectF tiles = item("tileView")->mapRectToScene(item("tileView")->boundingRect());
    QVERIFY2(QRectF(QPointF(0, 0), QSizeF(size)).contains(frame), "board frame leaves the window");
    QVERIFY2(frame.adjusted(-0.5, -0.5, 0.5, 0.5).contains(tiles), "tile grid overflows the board frame");
    // The grid uses most of the free space along its tighter axis.
    QVERIFY(tiles.width() >= frame.width() * 0.75 || tiles.height() >= frame.height() * 0.75);

    // Tiles stay clickable at every size: click a known group and see it resolve.
    controller()->setGridForTesting(makeGrid({
        {0, 0, 0},
        {1, 2, 3},
    }));
    window()->resize(size);
    QCoreApplication::processEvents();
    clickTile(0, 2);
    waitUntilIdle();
    QCOMPARE(controller()->score(), 9);

    const QString shotDir = qEnvironmentVariable("TAPOCALYPSE_E2E_SCREENSHOT_DIR");
    if (!shotDir.isEmpty()) {
        QMetaObject::invokeMethod(scoreDialog(), "close");
        controller()->reset();
        QTest::qWait(400);
        window()->grabWindow().save(QStringLiteral("%1/layout-%2x%3.png").arg(shotDir).arg(size.width()).arg(size.height()));
    }
}

int main(int argc, char** argv) {
    // Render without a display, in a resizable window like the desktop build.
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
    std::vector<char*> args(argv, argv + argc);
    char windowed[] = "--windowed";
    args.push_back(windowed);
    int appArgc = static_cast<int>(args.size());
    QGuiApplication app(appArgc, args.data());
    EndToEndTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_e2e.moc"
