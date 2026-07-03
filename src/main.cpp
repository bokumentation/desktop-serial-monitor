#include "serialporthandler.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  SerialPortHandler serialHandler;
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("serialHandler", &serialHandler);
  engine.loadFromModule("QtTest", "Main");
  return app.exec();
}
