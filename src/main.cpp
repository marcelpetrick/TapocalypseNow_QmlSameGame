/**
 * @file main.cpp
 * @brief Application entry point for the Tapocalypse desktop client.
 */

#include <qqml.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include "FrameProfiler.h"
#include "GameBoardController.h"
#include "HighScoreManager.h"

/**
 * @brief Start the Tapocalypse Qt application.
 * @param argc Process argument count.
 * @param argv Process argument vector.
 * @return Qt event-loop exit code.
 */
int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    qmlRegisterType<GameBoardController>("Tapocalypse", 1, 0, "GameBoardController");

    // Declared before the engine so it outlives every QML binding that reads it.
    HighScoreManager hsManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("highScoreManager", &hsManager);

    const QUrl url(QStringLiteral("qrc:/Tapocalypse/qml/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    // Profiling is entirely opt-in so normal startup and frame handling remain
    // unchanged unless explicitly requested by the environment.
    if (qEnvironmentVariable("TAPOCALYPSE_PROFILE") == QStringLiteral("1")) {
        bool durationValid = false;
        const double autoExitSeconds = qEnvironmentVariable("TAPOCALYPSE_PROFILE_SECONDS").toDouble(&durationValid);

        const auto roots = engine.rootObjects();
        if (!roots.isEmpty()) {
            if (auto* window = qobject_cast<QQuickWindow*>(roots.constFirst())) {
                new FrameProfiler(window, durationValid ? autoExitSeconds : 0.0);
            } else {
                qWarning().noquote() << "TAPOCALYPSE_FRAME error=root_is_not_qquickwindow";
            }
        }
    }

    return app.exec();
}
