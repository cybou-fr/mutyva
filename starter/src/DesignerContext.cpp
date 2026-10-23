#include "DesignerContext.h"
#include "DeclarativeRuntime.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

QString DesignerContext::instructions() {
    QFile file(":/runtime/designer-rules.txt");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(file.readAll());
}

bool DesignerContext::build(const QJsonObject &document, int generation, const QString &request,
                            QJsonObject &packet, QString &error) {
    if (!DeclarativeRuntime::validateDocument(document, error)) return false;
    if (generation < 1 || request.trimmed().isEmpty() || request.toUcs4().size() > 4000) {
        error = "Нужны сохранённое приложение и запрос от 1 до 4000 символов";
        return false;
    }
    const auto rules = instructions();
    if (rules.isEmpty()) { error = "Правила дизайнера недоступны"; return false; }
    const QJsonObject context{
        {"contextVersion", 1}, {"baseGeneration", generation},
        {"productConcept", QJsonObject{
            {"name", "MUTYVA"},
            {"purpose", "Локальная среда приложений из проверенных компонентов. AI проектирует; Qt исполняет сохранённую декларацию без AI."},
            {"currentScope", "Доверенный каталог содержит элементы интерфейса и локальные типизированные модели. Записи и значения не входят в AI-контекст."},
            {"layout", "flat-widget-list-with-parent-references; card/stack/row containers; max depth 8"}}},
        {"installedComponentCatalog", QJsonArray::fromVariantList(DeclarativeRuntime::catalog())},
        {"currentApplication", document},
        {"mutationSchema", DeclarativeRuntime::bundledSchema("mutation")},
        {"appSchema", DeclarativeRuntime::bundledSchema("app")},
        {"designerResponseSchema", DeclarativeRuntime::bundledSchema("designer-response")},
        {"userRequest", request.trimmed()}
    };
    // Keep trusted instructions separate from the JSON data that becomes model input.
    const QJsonObject result{{"instructions", rules}, {"context", context}};
    if (QJsonDocument(result).toJson(QJsonDocument::Compact).size() > 128 * 1024) {
        error = "Контекст превышает лимит 128 КБ";
        return false;
    }
    packet = result;
    error.clear();
    return true;
}
