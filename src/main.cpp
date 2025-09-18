#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "HighScoreManager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    HighScoreManager hsManager;
    engine.rootContext()->setContextProperty("highScoreManager", &hsManager);

    const QUrl url(QStringLiteral("qrc:/SameGame/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
