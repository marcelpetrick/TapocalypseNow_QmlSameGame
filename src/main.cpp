#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "GameLogicBridge.h"
#include "HighScoreManager.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    HighScoreManager hsManager;
    GameLogicBridge gameLogic;
    engine.rootContext()->setContextProperty("highScoreManager", &hsManager);
    engine.rootContext()->setContextProperty("gameLogic", &gameLogic);

    const QUrl url(QStringLiteral("qrc:/SameGame/qml/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject* obj, const QUrl& objUrl) {
            if (!obj && url == objUrl) QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
