#include "InteractionState.h"
#include <QJsonDocument>
#include <QJsonArray>

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
    auto store = std::make_unique<LocalDataStore>();
    LocalDataSnapshot loaded;
    QString error;
    const auto app = QJsonDocument::fromJson(m_document->documentJson().toUtf8()).object();
    if (!store->open(storagePath(), app.value("id").toString(), loaded, error)) return reject(error);
    m_store = std::move(store);
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
    if (widget(id).value("kind").toString() != "input" || text.toUcs4().size() > 500)
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
