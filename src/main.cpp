#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include "serialporthandler.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    SerialPortHandler serialHandler;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("serialHandler", &serialHandler);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/QtTest/Main.qml")));
    return app.exec();
}
