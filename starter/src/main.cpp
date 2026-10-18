#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>
#include <QQuickStyle>
#include <QIcon>
#include "WeatherService.h"
#include "AppDocument.h"
#include "InteractionState.h"
#include "BrandIdentity.h"
#include "ChatSession.h"
#include "AiChat.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("MUTYVA");
    QCoreApplication::setApplicationName("City Pulse Demo");
    // Keep the existing storage namespace; display identity is independent.
    QGuiApplication::setApplicationDisplayName("MUTYVA");
    QCoreApplication::setApplicationVersion(MUTYVA_VERSION);
    QGuiApplication::setDesktopFileName("org.mutyva.app");
    QGuiApplication::setWindowIcon(QIcon(":/branding/exports/mutyva-icon.png"));
    QQuickStyle::setStyle("Fusion");
    WeatherService weather;
    AppDocument document;
    InteractionState interaction(&document);
    BrandIdentity branding;
    ChatSession chatSession;
    AiChat aiChat(&document, &chatSession);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("weatherService", &weather);
    engine.rootContext()->setContextProperty("appDocument", &document);
    engine.rootContext()->setContextProperty("interactionState", &interaction);
    engine.rootContext()->setContextProperty("brandIdentity", &branding);
    engine.rootContext()->setContextProperty("chatSession", &chatSession);
    engine.rootContext()->setContextProperty("aiChat", &aiChat);
    engine.loadFromModule("Mutyva.Demo", "Main");
    if (engine.rootObjects().isEmpty()) return -1;
    weather.followDocument(&document);
    return app.exec();
}
