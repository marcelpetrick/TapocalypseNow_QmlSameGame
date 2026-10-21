#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include <QVector>

class QQuickWindow;

/**
 * Collects lightweight frame-swap timing samples for opt-in field profiling.
 *
 * Frames are timestamped directly on the thread that emits QQuickWindow::frameSwapped (the render thread with
 * the threaded render loop), so GUI-thread stalls do not distort the intervals. The hot path stores integer
 * nanosecond intervals only; sorting and formatting run in the periodic report on the GUI thread.
 */
class FrameProfiler final : public QObject {
   public:
    explicit FrameProfiler(QQuickWindow* window, double autoExitSeconds = 0.0);

    /**
     * @brief Format one report line from frame intervals.
     * @param intervalsNs Frame-to-frame intervals in nanoseconds, in any order.
     * @return A @c TAPOCALYPSE_FRAME line with frame count, fps, mean, percentiles, max and budget overruns.
     */
    static QByteArray summarize(QVector<qint64> intervalsNs);

   private:
    void recordFrame();
    void report();

    QTimer reportTimer_;
    QMutex mutex_;  // guards frameTimer_ and intervalsNs_, written on the render thread
    QElapsedTimer frameTimer_;
    QVector<qint64> intervalsNs_;
};
