/**
 * @file main.cpp
 * @brief Application entry point for the Tapocalypse desktop client.
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExtensionPlugin>
#include <QQuickWindow>

#include "FrameProfiler.h"
#include "HighScoreManager.h"

/**
 * @brief Start the Tapocalypse Qt application.
 * @param argc Process argument count.
 * @param argv Process argument vector.
 * @return Qt event-loop exit code.
 */
Q_IMPORT_QML_PLUGIN(TapocalypsePlugin)

int main(int argc, char* argv[]) {
    // Without a terminal (e.g. Qt Creator's output pane) Qt may route qDebug/console.log to the system journal.
    // Keep logs on stderr where IDEs and shells show them, unless the user chose otherwise.
    if (!qEnvironmentVariableIsSet("QT_FORCE_STDERR_LOGGING")) qputenv("QT_FORCE_STDERR_LOGGING", "1");

    QGuiApplication app(argc, argv);
    // The application name selects the highscore directory. It matches the binary name Qt used implicitly before,
    // so existing scoreboards stay where they are even if the executable is renamed.
    QCoreApplication::setApplicationName(QStringLiteral("tapocalypse"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("Tapocalypse"));
    QCoreApplication::setApplicationVersion(QStringLiteral(TAPOCALYPSE_VERSION));

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
