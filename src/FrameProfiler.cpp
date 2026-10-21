#include "FrameProfiler.h"

#include <QCoreApplication>
#include <QMutexLocker>
#include <QQuickWindow>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <numeric>

namespace {
constexpr int kReportIntervalMs = 1000;
constexpr qint64 kNanosecondsPerSecond = 1'000'000'000;
constexpr qint64 kNanosecondsPerMillisecond = 1'000'000;
constexpr double kFirstFrameBudgetMs = 16.67;
constexpr double kSecondFrameBudgetMs = 33.33;

void writeSummary(const QByteArray& summary) {
    // Write independently of Qt logging categories, which can compile out or
    // suppress informational messages in production builds.
    std::fwrite(summary.constData(), 1, static_cast<size_t>(summary.size()), stderr);
    std::fputc('\n', stderr);
    std::fflush(stderr);
}

double percentileMs(const QVector<qint64>& sortedIntervals, double percentile) {
    if (sortedIntervals.isEmpty()) {
        return 0.0;
    }

    // Nearest-rank percentile keeps the result an observed frame interval.
    const auto rank = static_cast<qsizetype>(std::ceil(percentile * static_cast<double>(sortedIntervals.size())));
    const qsizetype index = std::clamp<qsizetype>(rank - 1, 0, sortedIntervals.size() - 1);
    return static_cast<double>(sortedIntervals.at(index)) / kNanosecondsPerMillisecond;
}
}  // namespace

FrameProfiler::FrameProfiler(QQuickWindow* window, double autoExitSeconds) : QObject(window) {
    // Reserve well above a typical one-second refresh count without allowing
    // telemetry to grow indefinitely between reports.
    intervalsNs_.reserve(512);

    // Direct connection: record on the emitting (render) thread instead of when the GUI thread gets to it.
    connect(window, &QQuickWindow::frameSwapped, this, [this] { recordFrame(); }, Qt::DirectConnection);
    connect(&reportTimer_, &QTimer::timeout, this, [this] { report(); });
    reportTimer_.start(kReportIntervalMs);

    if (autoExitSeconds > 0.0 && std::isfinite(autoExitSeconds)) {
        const double milliseconds = autoExitSeconds * 1000.0;
        const int delayMs = static_cast<int>(std::clamp(milliseconds, 1.0, static_cast<double>(std::numeric_limits<int>::max())));
        QTimer::singleShot(delayMs, QCoreApplication::instance(), &QCoreApplication::quit);
    }
}

void FrameProfiler::recordFrame() {
    const QMutexLocker lock(&mutex_);
    if (!frameTimer_.isValid()) {
        frameTimer_.start();
        return;
    }

    intervalsNs_.append(frameTimer_.nsecsElapsed());
    frameTimer_.restart();
}

void FrameProfiler::report() {
    // Swap in an empty, pre-reserved buffer so frame collection continues while statistics are computed.
    QVector<qint64> samples;
    samples.reserve(512);
    {
        const QMutexLocker lock(&mutex_);
        intervalsNs_.swap(samples);
    }
    writeSummary(summarize(std::move(samples)));
}

QByteArray FrameProfiler::summarize(QVector<qint64> samples) {
    const qsizetype frames = samples.size();
    if (samples.isEmpty()) {
        return QByteArrayLiteral(
            "TAPOCALYPSE_FRAME frames=0 fps=0.00 mean_ms=0.000"
            " p50_ms=0.000 p95_ms=0.000 p99_ms=0.000 max_ms=0.000"
            " over_16_67=0 over_33_33=0");
    }

    const qint64 totalNs = std::accumulate(samples.cbegin(), samples.cend(), qint64{0});
    const double elapsedSeconds = static_cast<double>(totalNs) / kNanosecondsPerSecond;
    const double fps = elapsedSeconds > 0.0 ? static_cast<double>(frames) / elapsedSeconds : 0.0;
    const double meanMs = static_cast<double>(totalNs) / static_cast<double>(frames) / kNanosecondsPerMillisecond;
    const auto overFirstBudget =
        std::count_if(samples.cbegin(), samples.cend(), [](qint64 ns) { return static_cast<double>(ns) / kNanosecondsPerMillisecond > kFirstFrameBudgetMs; });
    const auto overSecondBudget =
        std::count_if(samples.cbegin(), samples.cend(), [](qint64 ns) { return static_cast<double>(ns) / kNanosecondsPerMillisecond > kSecondFrameBudgetMs; });

    std::sort(samples.begin(), samples.end());
    const double maxMs = static_cast<double>(samples.constLast()) / kNanosecondsPerMillisecond;

    return QStringLiteral(
               "TAPOCALYPSE_FRAME frames=%1 fps=%2 mean_ms=%3 p50_ms=%4 p95_ms=%5 "
               "p99_ms=%6 max_ms=%7 over_16_67=%8 over_33_33=%9")
        .arg(frames)
        .arg(fps, 0, 'f', 2)
        .arg(meanMs, 0, 'f', 3)
        .arg(percentileMs(samples, 0.50), 0, 'f', 3)
        .arg(percentileMs(samples, 0.95), 0, 'f', 3)
        .arg(percentileMs(samples, 0.99), 0, 'f', 3)
        .arg(maxMs, 0, 'f', 3)
        .arg(overFirstBudget)
        .arg(overSecondBudget)
        .toUtf8();
}
