/**
 * @file main.cpp
 * @brief Application entry point for the Tapocalypse desktop client.
 */

#include <qqml.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

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

    QQmlApplicationEngine engine;

    HighScoreManager hsManager;
    engine.rootContext()->setContextProperty("highScoreManager", &hsManager);

    const QUrl url(QStringLiteral("qrc:/Tapocalypse/qml/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
