#include "AppDocument.h"
#include "DeclarativeRuntime.h"
#include "DesignerContext.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QLockFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QMap>
#include <QStringList>

namespace {
QVariantList widgetList(const QJsonObject &document) {
    return document.value("widgets").toArray().toVariantList();
}
bool parse(const QString &json, QJsonObject &out, QString &error) {
    if (json.toUtf8().size() > 1024 * 1024) {
        error = "Документ превышает 1 МБ";
        return false;
    }
    QJsonParseError details;
    const auto doc = QJsonDocument::fromJson(json.toUtf8(), &details);
    if (details.error != QJsonParseError::NoError || !doc.isObject()) {
        error = "Ожидается JSON-объект: " + details.errorString();
        return false;
    }
    out = doc.object();
    return true;
}
}

AppDocument::AppDocument(QObject *parent, const QString &storageFolder) : QObject(parent) {
    m_folder = storageFolder.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/generations"
        : storageFolder;
    if (!loadDocument(":/examples/city-pulse.mutyva.json", m_document) ||
        !DeclarativeRuntime::validateDocument(m_document, m_error)) {
        m_storageReady = false;
        return;
    }
    if (!QDir().mkpath(m_folder)) {
        m_error = "Не удалось создать папку поколений";
        m_storageReady = false;
        return;
    }
    const auto head = m_folder + "/head.json";
    if (QFile::exists(head)) {
        QJsonObject meta, saved;
        if (loadDocument(head, meta)) {
            const int n = meta.value("generation").toInt(-1);
            if (n > 0 && loadDocument(generationPath(n), saved) &&
                DeclarativeRuntime::validateDocument(saved, m_error)) {
                m_generation = n;
                m_document = saved;
                if (saved.value("schemaVersion") == QJsonValue(1)) {
                    auto upgraded = saved;
                    upgraded.insert("schemaVersion", 2);
                    commit(upgraded); // Append migration; never rewrite the v1 snapshot.
                }
                return;
            }
        }
        if (m_error.isEmpty()) m_error = "Некорректный HEAD";
        m_error += ". Сохранение заблокировано: существующая история сохранена.";
        m_storageReady = false;
        return;
    }
    commit(m_document);
}

QString AppDocument::generationPath(int n) const {
    return m_folder + QString("/generation-%1.json").arg(n, 5, 10, QChar('0'));
}

bool AppDocument::loadDocument(const QString &path, QJsonObject &out) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { m_error = "Не удалось открыть: " + path; return false; }
    if (file.size() > 1024 * 1024) { m_error = "Документ превышает 1 МБ: " + path; return false; }
    return parse(QString::fromUtf8(file.readAll()), out, m_error);
}

QVariantList AppDocument::widgets() const { return widgetList(m_document); }
QVariantList AppDocument::previewWidgets() const { return widgetList(m_candidate); }
QVariantList AppDocument::catalog() const { return DeclarativeRuntime::catalog(); }
QString AppDocument::documentJson() const {
    return QString::fromUtf8(QJsonDocument(m_document).toJson(QJsonDocument::Indented));
}

bool AppDocument::reject(const QString &error) {
    m_error = error;
    emit documentChanged();
    return false;
}

bool AppDocument::commit(const QJsonObject &next) {
    if (!m_storageReady) return reject("Сохранение заблокировано из-за ошибки загрузки истории");
    QString validationError;
    if (!DeclarativeRuntime::validateDocument(next, validationError)) return reject(validationError);
    QLockFile lock(m_folder + "/write.lock");
    if (!lock.tryLock(0)) return reject("История занята другим экземпляром приложения");
    const auto headPath = m_folder + "/head.json";
    if (QFile::exists(headPath)) {
        QJsonObject meta;
        if (!loadDocument(headPath, meta)) return reject(m_error);
        if (meta.value("generation").toInt(-1) != m_generation)
            return reject("История изменена другим экземпляром. Перезапустите приложение.");
    } else if (m_generation > 0) return reject("HEAD отсутствует. Сохранение отменено.");
    int n = m_generation + 1;
    const QRegularExpression pattern("^generation-([0-9]+)\\.json$");
    for (const auto &name : QDir(m_folder).entryList({"generation-*.json"}, QDir::Files)) {
        const auto match = pattern.match(name);
        if (match.hasMatch()) n = qMax(n, match.captured(1).toInt() + 1);
    }
    QSaveFile snapshot(generationPath(n));
    const auto bytes = QJsonDocument(next).toJson(QJsonDocument::Indented);
    if (!snapshot.open(QIODevice::WriteOnly) || snapshot.write(bytes) != bytes.size() || !snapshot.commit())
        return reject("Не удалось сохранить поколение");
    QSaveFile head(headPath);
    const auto meta = QJsonDocument(QJsonObject{{"generation", n}}).toJson();
    if (!head.open(QIODevice::WriteOnly) || head.write(meta) != meta.size() || !head.commit())
        return reject("Не удалось обновить HEAD; предыдущее поколение сохранено");
    m_document = next;
    m_generation = n;
    m_contextGeneration = 0;
    m_designerContext.clear();
    m_designerMessage.clear();
    emit designerChanged();
    m_candidate = {};
    m_summary.clear();
    m_error.clear();
    emit documentChanged();
    return true;
}

bool AppDocument::stage(const QJsonObject &proposed, const QString &summary) {
    QString error;
    if (!DeclarativeRuntime::validateDocument(proposed, error)) return reject(error);
    auto candidate = proposed;
    candidate.insert("schemaVersion", 2);
    if (!DeclarativeRuntime::validateDocument(candidate, error)) return reject(error);
    if (candidate.value("id") != m_document.value("id")) return reject("Нельзя изменить ID приложения");
    if (candidate.value("permissions") != m_document.value("permissions") ||
        candidate.value("dataSources") != m_document.value("dataSources"))
        return reject("В этом этапе можно менять интерфейс; источники и разрешения должны совпадать");
    if (candidate == m_document) return reject("Изменений нет");
    QStringList changes;
    QMap<QString, QJsonObject> before, after;
    QStringList oldOrder, newOrder;
    for (const auto &entry : m_document.value("widgets").toArray()) {
        const auto w = entry.toObject();
        const auto id = w.value("id").toString();
        before.insert(id, w); oldOrder.append(id);
    }
    for (const auto &entry : candidate.value("widgets").toArray()) {
        const auto w = entry.toObject();
        const auto id = w.value("id").toString();
        after.insert(id, w); newOrder.append(id);
        if (!before.contains(id)) changes << "+ Добавить " + id + " (" + w.value("kind").toString() + ")";
        else {
            if (before[id].value("kind") != w.value("kind"))
                changes << "Тип " + id + ": " + before[id].value("kind").toString() + " → " + w.value("kind").toString();
            if (before[id].value("title") != w.value("title"))
                changes << "Заголовок " + id + ": «" + before[id].value("title").toString() + "» → «" + w.value("title").toString() + "»";
            for (const auto &property : {"text", "value", "placeholder", "items", "action", "parent"})
                if (before[id].value(property) != w.value(property))
                    changes << "Изменить " + id + "." + property + ": " +
                        QString::fromUtf8(QJsonDocument(QJsonArray{w.value(property)}).toJson(QJsonDocument::Compact));
        }
    }
    for (const auto &id : oldOrder)
        if (!after.contains(id)) changes << "− Удалить " + id + " (значения сеанса будут потеряны)";
    if (oldOrder != newOrder) changes << "Порядок: " + newOrder.join(" → ");
    if (candidate.value("name") != m_document.value("name")) changes << "Имя: " + candidate.value("name").toString();
    if (candidate.value("description") != m_document.value("description")) changes << "Описание приложения изменено";
    m_candidate = candidate;
    m_summary = summary + "\n" + changes.join('\n');
    m_error.clear();
    emit documentChanged();
    return true;
}

void AppDocument::cancelPreview() {
    m_candidate = {};
    m_summary.clear();
    m_error.clear();
    emit documentChanged();
}

bool AppDocument::previewMutation(const QString &json) {
    cancelPreview();
    QJsonObject proposal, candidate;
    QString error;
    if (!parse(json, proposal, error) || !DeclarativeRuntime::mutate(m_document, proposal, candidate, error))
        return reject(error);
    return stage(candidate, proposal.value("intent").toString("Изменение интерфейса"));
}

bool AppDocument::previewDocument(const QString &json) {
    cancelPreview();
    QJsonObject next;
    QString error;
    if (!parse(json, next, error)) return reject(error);
    return stage(next, "Замена декларации интерфейса");
}

bool AppDocument::propose(const QJsonObject &operation, const QString &summary) {
    const QJsonObject proposal{{"schemaVersion", 2}, {"appId", m_document.value("id")},
                               {"intent", summary}, {"operations", QJsonArray{operation}}};
    return previewMutation(QString::fromUtf8(QJsonDocument(proposal).toJson()));
}

bool AppDocument::addWidget(const QString &kind) {
    const auto entries = catalog();
    QString title;
    QJsonObject defaults;
    for (const auto &entry : entries)
        if (entry.toMap().value("kind").toString() == kind) {
            title = entry.toMap().value("title").toString();
            defaults = QJsonObject::fromVariantMap(entry.toMap().value("defaults").toMap());
        }
    const auto widgets = m_document.value("widgets").toArray();
    QString id;
    for (int i = 1; i <= 101; ++i) {
        id = "widget" + QString::number(i);
        bool found = false;
        for (const auto &w : widgets) if (w.toObject().value("id").toString() == id) found = true;
        if (!found) break;
    }
    defaults.insert("id", id); defaults.insert("kind", kind); defaults.insert("title", title);
    return propose({{"op", "addWidget"}, {"widget", defaults}},
                   "Добавить: " + title);
}

bool AppDocument::addHumidityCard() {
    QJsonObject proposal;
    if (!loadDocument(":/examples/mutations/add-humidity-card.json", proposal)) return reject(m_error);
    return previewMutation(QString::fromUtf8(QJsonDocument(proposal).toJson()));
}

bool AppDocument::removeWidget(const QString &id) {
    return propose({{"op", "removeWidget"}, {"id", id}}, "Удалить виджет: " + id);
}

bool AppDocument::renameWidget(const QString &id, const QString &title) {
    return propose({{"op", "updateProperty"}, {"id", id}, {"path", "title"}, {"value", title}},
                   "Заголовок " + id + ": " + title);
}

bool AppDocument::updateWidgetProperty(const QString &id, const QString &path, const QVariant &value) {
    return propose({{"op", "updateProperty"}, {"id", id}, {"path", path}, {"value", QJsonValue::fromVariant(value)}},
                   "Изменить свойство " + id + "." + path);
}

bool AppDocument::moveInto(const QString &id, const QString &parent) {
    return propose({{"op", "moveWidget"}, {"id", id}, {"after", ""}, {"parent", parent}},
                   "Переместить " + id + (parent.isEmpty() ? " в корень" : " в контейнер " + parent));
}

bool AppDocument::moveWidget(const QString &id, const QString &after) {
    return propose({{"op", "moveWidget"}, {"id", id}, {"after", after}},
                   "Переместить " + id + (after.isEmpty() ? " в начало" : " после " + after));
}

bool AppDocument::applyPreview() {
    if (!hasPreview()) return reject("Нет изменений для применения");
    return commit(m_candidate);
}

bool AppDocument::rollback() {
    cancelPreview();
    if (m_generation < 2) return reject("Предыдущего сохранения нет");
    QJsonObject prior;
    if (!loadDocument(generationPath(m_generation - 1), prior)) return reject(m_error);
    return stage(prior, "Восстановить предыдущее сохранение как новое поколение");
}

bool AppDocument::resetDemo() {
    cancelPreview();
    QJsonObject initial;
    if (!loadDocument(":/examples/city-pulse.mutyva.json", initial)) return reject(m_error);
    return stage(initial, "Восстановить исходный интерфейс");
}

bool AppDocument::previewPaletteDemo() {
    cancelPreview();
    QJsonObject demo;
    if (!loadDocument(":/examples/general-workspace.mutyva.json", demo)) return reject(m_error);
    return stage(demo, "Открыть пример общей палитры: заметки и погода");
}

bool AppDocument::prepareDesignerContext(const QString &request) {
    QJsonObject packet;
    QString error;
    m_contextGeneration = 0;
    m_designerContext.clear();
    m_designerMessage.clear();
    if (hasPreview() || !DesignerContext::build(m_document, m_generation, request, packet, error)) {
        if (hasPreview()) error = "Сначала примените или отмените предпросмотр";
        emit designerChanged();
        return reject(error);
    }
    m_contextGeneration = m_generation;
    m_designerContext = QString::fromUtf8(QJsonDocument(packet).toJson(QJsonDocument::Indented));
    m_designerMessage = "Контекст подготовлен локально. Отправки в AI ещё нет.";
    m_error.clear();
    emit documentChanged();
    emit designerChanged();
    return true;
}

bool AppDocument::previewDesignerResponse(const QString &json) {
    cancelPreview();
    m_designerMessage.clear();
    emit designerChanged();
    if (m_contextGeneration < 1 || m_contextGeneration != m_generation)
        return reject("Сначала подготовьте контекст для текущего поколения");
    QJsonObject response;
    QString error;
    if (!parse(json, response, error) || !DeclarativeRuntime::validateDesignerResponse(response, error))
        return reject(error);
    if (response.value("status").toString() == "proposal") {
        if (!previewMutation(QString::fromUtf8(QJsonDocument(response.value("mutation").toObject()).toJson())))
            return false;
    }
    m_designerMessage = response.value("message").toString();
    emit designerChanged();
    return true;
}
