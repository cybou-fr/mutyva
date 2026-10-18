#include "AppDocument.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QStandardPaths>

AppDocument::AppDocument(QObject *parent) : QObject(parent) {
    m_folder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/generations";
    QDir().mkpath(m_folder);
    const QString head = m_folder + "/head.json";
    if (QFile::exists(head)) {
        QJsonObject meta;
        if (loadDocument(head, meta)) {
            const int n = meta.value("generation").toInt();
            QJsonObject saved;
            if (n > 0 && loadDocument(generationPath(n), saved)) {
                m_generation = n; m_document = saved; return;
            }
        }
    }
    QJsonObject initial;
    if (loadDocument(":/examples/city-pulse.mutyva.json", initial)) commit(initial);
}

QString AppDocument::generationPath(int n) const {
    return m_folder + QString("/generation-%1.json").arg(n, 5, 10, QChar('0'));
}

bool AppDocument::loadDocument(const QString &path, QJsonObject &out) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { m_error = "Не удалось открыть: " + path; return false; }
    QJsonParseError error;
    const QJsonDocument parsed = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !parsed.isObject()) {
        m_error = "Ошибка JSON: " + path; return false;
    }
    out = parsed.object(); return true;
}

QVariantList AppDocument::widgets() const {
    QVariantList result;
    const auto list = m_document.value("widgets").toArray();
    for (const auto &item : list) result.append(item.toObject().toVariantMap());
    return result;
}

bool AppDocument::commit(const QJsonObject &next) {
    if (!next.value("widgets").isArray() || next.value("schemaVersion").toInt() != 1) {
        m_error = "Недопустимая декларация приложения"; emit documentChanged(); return false;
    }
    const int n = m_generation + 1;
    QSaveFile snapshot(generationPath(n));
    if (!snapshot.open(QIODevice::WriteOnly)) { m_error = "Не удалось сохранить поколение"; emit documentChanged(); return false; }
    snapshot.write(QJsonDocument(next).toJson(QJsonDocument::Indented));
    if (!snapshot.commit()) { m_error = "Ошибка записи поколения"; emit documentChanged(); return false; }
    QSaveFile head(m_folder + "/head.json");
    if (!head.open(QIODevice::WriteOnly)) { m_error = "Не удалось открыть HEAD"; emit documentChanged(); return false; }
    head.write(QJsonDocument(QJsonObject{{"generation", n}}).toJson());
    if (!head.commit()) { m_error = "Не удалось обновить HEAD"; emit documentChanged(); return false; }
    m_document = next; m_generation = n; m_error.clear(); emit documentChanged(); return true;
}

bool AppDocument::addHumidityCard() {
    auto widgets = m_document.value("widgets").toArray();
    for (const auto &w : widgets) {
        if (w.toObject().value("id").toString() == "humidity") return false;
    }
    const QJsonObject humidity{{"id", "humidity"}, {"kind", "humidity-card"}, {"title", "Влажность воздуха"}};
    int insertionIndex = widgets.size();
    for (qsizetype i = 0; i < widgets.size(); ++i) {
        if (widgets.at(i).toObject().value("id").toString() == "forecast") {
            insertionIndex = static_cast<int>(i + 1);
            break;
        }
    }
    widgets.insert(insertionIndex, humidity);
    QJsonObject next = m_document;
    next.insert("widgets", widgets);
    return commit(next);
}

bool AppDocument::rollback() {
    if (m_generation < 2) return false;
    QJsonObject prior;
    if (!loadDocument(generationPath(m_generation - 1), prior)) { emit documentChanged(); return false; }
    // Rollback is itself a NEW immutable generation, preserving full history.
    return commit(prior);
}

bool AppDocument::resetDemo() {
    QJsonObject initial;
    if (!loadDocument(":/examples/city-pulse.mutyva.json", initial)) { emit documentChanged(); return false; }
    return commit(initial);
}
