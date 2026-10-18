#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include "WeatherService.h"
#include "AppDocument.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("MUTYVA");
    QCoreApplication::setApplicationName("City Pulse Demo");
    WeatherService weather;
    AppDocument document;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("weatherService", &weather);
    engine.rootContext()->setContextProperty("appDocument", &document);
    engine.loadFromModule("Mutyva.Demo", "Main");
    if (engine.rootObjects().isEmpty()) return -1;
    weather.lookup("Paris");
    return app.exec();
}
