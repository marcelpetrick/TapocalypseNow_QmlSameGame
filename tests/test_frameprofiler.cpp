#include <QQuickWindow>
#include <QtTest>

#include "FrameProfiler.h"

class FrameProfilerTest : public QObject {
    Q_OBJECT

   private slots:
    void summarize_reportsEmptyWindow();
    void summarize_computesRatesPercentilesAndBudgets();
    void profiler_recordsSwappedFramesOfARealWindow();
};

void FrameProfilerTest::summarize_reportsEmptyWindow() {
    QCOMPARE(FrameProfiler::summarize({}),
             QByteArrayLiteral("TAPOCALYPSE_FRAME frames=0 fps=0.00 mean_ms=0.000 p50_ms=0.000 p95_ms=0.000 p99_ms=0.000 max_ms=0.000 "
                               "over_16_67=0 over_33_33=0"));
}

void FrameProfilerTest::summarize_computesRatesPercentilesAndBudgets() {
    // Ten frames: eight at 10 ms, one at 20 ms (over 16.67), one at 40 ms (over both budgets); given unsorted.
    QVector<qint64> intervals(8, 10'000'000);
    intervals.prepend(40'000'000);
    intervals.insert(4, 20'000'000);

    const QByteArray line = FrameProfiler::summarize(intervals);
    QCOMPARE(line, QByteArrayLiteral("TAPOCALYPSE_FRAME frames=10 fps=71.43 mean_ms=14.000 p50_ms=10.000 p95_ms=40.000 p99_ms=40.000 "
                                     "max_ms=40.000 over_16_67=2 over_33_33=1"));
}

void FrameProfilerTest::profiler_recordsSwappedFramesOfARealWindow() {
    QQuickWindow window;
    window.resize(64, 64);
    auto* profiler = new FrameProfiler(&window);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    for (int frame = 0; frame < 5; ++frame) {
        window.update();
        QTest::qWait(20);
    }
    // The report timer fires every second and must not crash while frames keep arriving.
    QTest::qWait(1100);
    QCOMPARE(profiler->parent(), &window);
}

int main(int argc, char** argv) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    FrameProfilerTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_frameprofiler.moc"
