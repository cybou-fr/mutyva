#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QCommandLineParser>
#include <QTimer>
#include <QDir>
#include "ChatSession.h"
#include "LiveAiCheck.h"

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("MUTYVA");
    QCoreApplication::setApplicationName("City Pulse Demo"); // Reuse the host/client registration; never import tokens.
    QGuiApplication::setApplicationDisplayName("MUTYVA — Проверка AI");
    QQuickStyle::setStyle("Fusion");
    app.setQuitOnLastWindowClosed(false);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"report", "Path to the JSON result", "path", QDir::current().absoluteFilePath("live-ai-report.json")});
    parser.process(app);
    ChatSession session;
    LiveAiCheck check(&session, parser.value("report"));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("chatSession", &session);
    engine.rootContext()->setContextProperty("liveCheck", &check);
    QObject::connect(&check, &LiveAiCheck::completed, &app, [&app](int code) { QTimer::singleShot(2000, &app, [&app, code] { app.exit(code); }); });
    QObject::connect(&app, &QGuiApplication::lastWindowClosed, &check, &LiveAiCheck::cancel);
    engine.load(QUrl("qrc:/live/tests/LiveAiCheck.qml"));
    if (engine.rootObjects().isEmpty()) return 3;
    QTimer::singleShot(0, &check, [&] { check.start(); if (!check.finished()) session.signIn(); });
    return app.exec();
}
