#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QVector>

class QQuickWindow;

/**
 * Collects lightweight frame-swap timing samples for opt-in field profiling.
 *
 * The hot path stores integer nanosecond intervals only. More expensive work
 * (sorting and formatting) is deferred to the periodic reporting timer.
 */
class FrameProfiler final : public QObject {
   public:
    explicit FrameProfiler(QQuickWindow* window, double autoExitSeconds = 0.0);

   private:
    void recordFrame();
    void report();

    QElapsedTimer frameTimer_;
    QTimer reportTimer_;
    QVector<qint64> intervalsNs_;
};
