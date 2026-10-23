#include "InteractionState.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDate>
#include <QSet>
#include <algorithm>
#include <QtMath>

InteractionState::InteractionState(AppDocument *document, QObject *parent)
    : QObject(parent), m_document(document) {
    connect(document, &AppDocument::documentChanged, this, [this] { ++m_revision; emit changed(); });
    reload();
}

QJsonObject InteractionState::widget(const QString &id) const {
    for (const auto &entry : m_document->widgets()) {
        const auto w = QJsonObject::fromVariantMap(entry.toMap());
        if (w.value("id").toString() == id) return w;
    }
    return {};
}

bool InteractionState::reload() {
    m_ready = false;
    m_store.reset();
    m_records.reset();
    auto store = std::make_unique<LocalDataStore>();
    LocalDataSnapshot loaded;
    QString error;
    const auto app = QJsonDocument::fromJson(m_document->documentJson().toUtf8()).object();
    if (!store->open(storagePath(), app.value("id").toString(), loaded, error)) return reject(error);
    auto records = std::make_unique<RecordStore>();
    if (!records->open(storagePath(), app.value("id").toString(), error)) return reject(error);
    m_store = std::move(store);
    m_records = std::move(records);
    m_snapshot = loaded;
    m_ready = true;
    m_error.clear(); ++m_revision; emit changed();
    return true;
}

QString InteractionState::value(const QString &id, const QString &fallback) const { return m_snapshot.inputs.value(id, fallback); }
QVariantList InteractionState::items(const QString &id, const QVariantList &fallback) const { return m_snapshot.lists.value(id, fallback); }

bool InteractionState::reject(const QString &error) { m_error = error; emit changed(); return false; }

bool InteractionState::persist(LocalDataSnapshot &candidate) {
    QString error;
    if (!m_store || !m_store->save(candidate, error)) {
        m_ready = false;
        return reject(error.isEmpty() ? "Хранилище недоступно. Нажмите «Перечитать данные»" : error);
    }
    m_snapshot = candidate;
    m_error.clear(); ++m_revision; emit changed();
    return true;
}

bool InteractionState::setValue(const QString &id, const QString &text) {
    if (m_document->hasPreview()) return reject("В предпросмотре действия отключены");
    if (!m_ready) return reject("Хранилище недоступно. Нажмите «Перечитать данные»");
    const auto fieldWidget = widget(id);
    const auto kind = fieldWidget.value("kind").toString();
    if ((kind != "input" && kind != "checkbox" && kind != "select" && kind != "date-picker") ||
        text.toUcs4().size() > 500 ||
        (kind == "checkbox" && text != "true" && text != "false") ||
        (kind == "select" && !fieldWidget.value("options").toArray().contains(text)) ||
        (kind == "date-picker" && !text.isEmpty() && !QDate::fromString(text, Qt::ISODate).isValid()))
        return reject("Недопустимое поле ввода или длина значения");
    auto next = m_snapshot;
    next.inputs.insert(id, text);
    return persist(next);
}

bool InteractionState::trigger(const QString &buttonId) {
    if (m_document->hasPreview()) return reject("В предпросмотре действия отключены");
    if (!m_ready) return reject("Хранилище недоступно. Нажмите «Перечитать данные»");
    const auto button = widget(buttonId);
    if (button.value("kind").toString() != "button") return reject("Кнопка не найдена");
    const auto action = button.value("action").toObject();
    const auto targetId = action.value("target").toString();
    const auto target = widget(targetId);
    if (target.value("kind").toString() != "list") return reject("Список назначения не найден");
    const auto kind = action.value("kind").toString();
    auto next = m_snapshot;
    if (kind == "clear-list") next.lists.insert(targetId, {});
    else if (kind == "append-item") {
        const auto inputId = action.value("input").toString();
        const auto input = widget(inputId);
        if (input.value("kind").toString() != "input") return reject("Поле источника не найдено");
        const auto text = value(inputId, input.value("value").toString()).trimmed();
        if (text.isEmpty()) return reject("Сначала введите текст");
        auto list = items(targetId, target.value("items").toArray().toVariantList());
        if (list.size() >= 50) return reject("В списке уже 50 элементов");
        list.append(text);
        next.lists.insert(targetId, list);
        next.inputs.insert(inputId, {});
    } else return reject("Действие не поддерживается");
    return persist(next);
}

QJsonObject InteractionState::model(const QString &id) const {
    const auto document = QJsonDocument::fromJson(m_document->documentJson().toUtf8()).object();
    for (const auto &entry : document.value("models").toArray())
        if (entry.toObject().value("id").toString() == id) return entry.toObject();
    return {};
}

QVariantList InteractionState::modelFields(const QString &modelId) const {
    return model(modelId).value("fields").toArray().toVariantList();
}

QVariantList InteractionState::records(const QString &widgetId) {
    const auto definition = widget(widgetId);
    if (definition.value("kind") != "record-list" || !m_ready || !m_records) return {};
    const auto modelId = definition.value("model").toString();
    if (model(modelId).isEmpty()) return {};
    QJsonArray loaded;
    QString error;
    if (!m_records->list(modelId, loaded, error)) { reject(error); return {}; }
    const auto filter = definition.value("filter").toObject();
    const auto order = definition.value("sort").toObject();
    QList<QJsonObject> rows;
    for (const auto &entry : loaded) {
        const auto row = entry.toObject();
        bool include = true;
        if (!filter.isEmpty()) {
            const auto value = row.value(filter.value("field").toString());
            const auto expected = filter.value("value");
            const auto op = filter.value("op").toString();
            if (op == "equals") include = value == expected;
            else if (value.isUndefined() || value.isNull()) include = false;
            else if (value.isDouble() && expected.isDouble())
                include = op == "before" ? value.toDouble() < expected.toDouble() : value.toDouble() > expected.toDouble();
            else if (value.isString() && expected.isString())
                include = op == "before" ? value.toString() < expected.toString() : value.toString() > expected.toString();
            else include = false;
        }
        if (include) rows.append(row);
    }
    if (!order.isEmpty()) {
        const auto field = order.value("field").toString();
        const bool descending = order.value("descending").toBool();
        std::stable_sort(rows.begin(), rows.end(), [&](const QJsonObject &a, const QJsonObject &b) {
            const auto av = a.value(field), bv = b.value(field);
            if (av == bv) return false;
            bool less;
            if (av.isUndefined() || av.isNull()) less = false;
            else if (bv.isUndefined() || bv.isNull()) less = true;
            else if (av.isDouble() && bv.isDouble()) less = av.toDouble() < bv.toDouble();
            else if (av.isBool() && bv.isBool()) less = !av.toBool() && bv.toBool();
            else less = av.toVariant().toString() < bv.toVariant().toString();
            return descending ? !less : less;
        });
    }
    QVariantList result;
    for (const auto &row : rows) result.append(row.toVariantMap());
    return result;
}

QVariantMap InteractionState::selectedRecord(const QString &modelId) {
    if (!m_ready || !m_records || model(modelId).isEmpty()) return {};
    const auto id = m_selectedRecords.value(modelId);
    if (id.isEmpty()) return {};
    QJsonArray rows;
    QString error;
    if (!m_records->list(modelId, rows, error)) { reject(error); return {}; }
    for (const auto &entry : rows)
        if (entry.toObject().value("_id").toString() == id) return entry.toObject().toVariantMap();
    return {};
}

bool InteractionState::selectRecord(const QString &widgetId, const QString &recordId) {
    if (m_document->hasPreview() || !m_ready) return reject("Выбор недоступен во время предпросмотра");
    const auto w = widget(widgetId);
    if (w.value("kind") != "record-list") return reject("Список записей не найден");
    const auto id = w.value("model").toString();
    if (model(id).isEmpty()) return reject("Модель не найдена");
    if (!recordId.isEmpty()) {
        QJsonArray rows; QString error;
        if (!m_records || !m_records->list(id, rows, error)) return reject(error);
        bool found = false;
        for (const auto &entry : rows) if (entry.toObject().value("_id") == recordId) found = true;
        if (!found) return reject("Запись не найдена");
    }
    m_selectedRecords.insert(id, recordId);
    ++m_revision; emit changed();
    return true;
}

bool InteractionState::normalize(const QString &modelId, const QVariantMap &values, QJsonObject &result) {
    const auto descriptor = model(modelId);
    if (descriptor.isEmpty()) return reject("Модель не найдена");
    QSet<QString> fields;
    QJsonObject candidate;
    for (const auto &entry : descriptor.value("fields").toArray()) {
        const auto field = entry.toObject();
        const auto id = field.value("id").toString();
        const auto kind = field.value("type").toString();
        fields.insert(id);
        const bool supplied = values.contains(id);
        QJsonValue value = supplied ? QJsonValue::fromVariant(values.value(id)) : field.value("default");
        if (value.isUndefined() || value.isNull() || (kind == "date" && value == QJsonValue(""))) {
            if (field.value("required").toBool()) return reject("Заполните обязательное поле " + id);
            continue;
        }
        bool valid = false;
        if (kind == "text") valid = value.isString() && value.toString().size() <= 1000;
        else if (kind == "integer") valid = value.isDouble() && value.toDouble() == qFloor(value.toDouble());
        else if (kind == "boolean") valid = value.isBool();
        else if (kind == "date") valid = value.isString() && QDate::fromString(value.toString(), Qt::ISODate).isValid();
        if (!valid || (field.value("required").toBool() && value == QJsonValue("")))
            return reject("Неверное значение поля " + id);
        candidate.insert(id, value);
    }
    for (auto it = values.begin(); it != values.end(); ++it)
        if (!fields.contains(it.key())) return reject("Неизвестное поле " + it.key());
    result = candidate;
    return true;
}
bool InteractionState::recordError(const QString &message) {
    m_ready = false;
    return reject(message);
}

bool InteractionState::createRecord(const QString &widgetId, const QVariantMap &values) {
    if (!m_ready || m_document->hasPreview()) return reject("Создание записи сейчас недоступно");
    const auto w = widget(widgetId);
    if (w.value("kind") != "record-form") return reject("Форма не найдена");
    QJsonObject clean;
    const auto modelId = w.value("model").toString();
    if (!normalize(modelId, values, clean)) return false;
    QString id, error;
    if (!m_records || !m_records->create(modelId, clean, id, error)) return recordError(error);
    m_selectedRecords.remove(modelId);
    m_error.clear(); ++m_revision; emit changed();
    return true;
}
bool InteractionState::updateRecord(const QString &widgetId, const QString &recordId, const QVariantMap &values) {
    if (!m_ready || m_document->hasPreview()) return reject("Изменение записи сейчас недоступно");
    const auto w = widget(widgetId);
    if (w.value("kind") != "record-form") return reject("Форма не найдена");
    QJsonObject clean;
    const auto modelId = w.value("model").toString();
    if (!normalize(modelId, values, clean)) return false;
    QString error;
    if (!m_records || !m_records->update(modelId, recordId, clean, error)) return recordError(error);
    m_error.clear(); ++m_revision; emit changed();
    return true;
}
bool InteractionState::deleteRecord(const QString &widgetId, const QString &recordId) {
    if (!m_ready || m_document->hasPreview()) return reject("Удаление записи сейчас недоступно");
    const auto w = widget(widgetId);
    if (w.value("kind") != "record-list") return reject("Список записей не найден");
    const auto modelId = w.value("model").toString();
    if (model(modelId).isEmpty()) return reject("Модель не найдена");
    QString error;
    if (!m_records || !m_records->remove(modelId, recordId, error)) return recordError(error);
    if (m_selectedRecords.value(modelId) == recordId) m_selectedRecords.remove(modelId);
    m_error.clear(); ++m_revision; emit changed();
    return true;
}

bool InteractionState::clearSelectedRecord(const QString &formWidgetId) {
    if (!m_ready || m_document->hasPreview()) return reject("Сброс выбора сейчас недоступен");
    const auto w = widget(formWidgetId);
    if (w.value("kind") != "record-form") return reject("Форма не найдена");
    m_selectedRecords.remove(w.value("model").toString());
    ++m_revision; emit changed();
    return true;
}
