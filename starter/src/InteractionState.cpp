#include "InteractionState.h"
#include <QJsonDocument>
#include <QJsonArray>

InteractionState::InteractionState(AppDocument *document, QObject *parent)
    : QObject(parent), m_document(document) {
    connect(document, &AppDocument::documentChanged, this, &InteractionState::reconcile);
}

QJsonObject InteractionState::widget(const QString &id) const {
    for (const auto &entry : m_document->widgets()) {
        const auto w = QJsonObject::fromVariantMap(entry.toMap());
        if (w.value("id").toString() == id) return w;
    }
    return {};
}

void InteractionState::reconcile() {
    bool removed = false;
    for (auto it = m_inputs.begin(); it != m_inputs.end();) {
        if (widget(it.key()).value("kind").toString() != "input") { it = m_inputs.erase(it); removed = true; }
        else ++it;
    }
    for (auto it = m_lists.begin(); it != m_lists.end();) {
        if (widget(it.key()).value("kind").toString() != "list") { it = m_lists.erase(it); removed = true; }
        else ++it;
    }
    if (removed) { ++m_revision; emit changed(); }
}

QString InteractionState::value(const QString &id, const QString &fallback) const { return m_inputs.value(id, fallback); }
QVariantList InteractionState::items(const QString &id, const QVariantList &fallback) const { return m_lists.value(id, fallback); }

bool InteractionState::reject(const QString &error) { m_error = error; emit changed(); return false; }

bool InteractionState::setValue(const QString &id, const QString &text) {
    if (m_document->hasPreview()) return reject("В предпросмотре действия отключены");
    if (widget(id).value("kind").toString() != "input" || text.toUcs4().size() > 500)
        return reject("Недопустимое поле ввода или длина значения");
    m_inputs.insert(id, text);
    m_error.clear(); ++m_revision; emit changed();
    return true;
}

bool InteractionState::trigger(const QString &buttonId) {
    if (m_document->hasPreview()) return reject("В предпросмотре действия отключены");
    const auto button = widget(buttonId);
    if (button.value("kind").toString() != "button") return reject("Кнопка не найдена");
    const auto action = button.value("action").toObject();
    const auto targetId = action.value("target").toString();
    const auto target = widget(targetId);
    if (target.value("kind").toString() != "list") return reject("Список назначения не найден");
    const auto kind = action.value("kind").toString();
    if (kind == "clear-list") m_lists.insert(targetId, {});
    else if (kind == "append-item") {
        const auto inputId = action.value("input").toString();
        const auto input = widget(inputId);
        if (input.value("kind").toString() != "input") return reject("Поле источника не найдено");
        const auto text = value(inputId, input.value("value").toString()).trimmed();
        if (text.isEmpty()) return reject("Сначала введите текст");
        auto list = items(targetId, target.value("items").toArray().toVariantList());
        if (list.size() >= 50) return reject("В списке уже 50 элементов");
        list.append(text);
        m_lists.insert(targetId, list);
        m_inputs.insert(inputId, {});
    } else return reject("Действие не поддерживается");
    m_error.clear(); ++m_revision; emit changed();
    return true;
}
