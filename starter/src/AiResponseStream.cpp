#include "AiResponseStream.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStringList>

namespace {
struct FinalText {
    QString text, reason;
    int messages = 0, finals = 0, commentary = 0;
};
FinalText selectFinal(const QJsonArray &items) {
    FinalText result;
    QStringList finals, plain;
    for (const auto &value : items) {
        const auto item = value.toObject();
        if (item.value("type") == "reasoning") continue;
        if (item.value("type") != "message" || item.value("role") != "assistant" ||
            (item.contains("status") && item.value("status") != "completed") || !item.value("content").isArray()) {
            result.reason = "unsupported_output_item"; return result;
        }
        ++result.messages;
        QString text;
        for (const auto &part : item.value("content").toArray()) {
            const auto content = part.toObject();
            if (content.value("type") != "output_text" || !content.value("text").isString()) {
                result.reason = content.value("type") == "refusal" ? "refusal" : "unsupported_output_content"; return result;
            }
            text += content.value("text").toString();
        }
        const auto phase = item.value("phase").toString();
        if (phase == "commentary") ++result.commentary;
        else if (phase == "final_answer") { ++result.finals; finals.append(text); }
        else if (phase.isEmpty()) plain.append(text);
        else { result.reason = "unsupported_message_phase"; return result; }
    }
    if (finals.size() == 1) result.text = finals.first();
    else if (finals.size() > 1) result.reason = "ambiguous_final_answer";
    else if (!plain.isEmpty()) result.text = plain.last();
    return result;
}
int boundedIndex(const QJsonObject &object, const QString &key, int maximum) {
    const auto value = object.value(key);
    if (!value.isDouble() || value.toDouble() != value.toInt(-1)) return -1;
    const int index = value.toInt(-1);
    return index >= 0 && index < maximum ? index : -1;
}
}

QString AiResponseStream::selectModel(const QJsonObject &catalog) {
    for (const auto &entry : catalog.value("models").toArray()) {
        const auto model = entry.toObject();
        const auto slug = model.value("slug").toString();
        if (model.value("visibility") == "list" && !slug.isEmpty() && slug.size() < 200) return slug;
    }
    return {};
}
bool AiResponseStream::feed(const QByteArray &bytes) {
    if (!m_error.isEmpty()) return false;
    m_total += bytes.size();
    if (m_total > 2 * 1024 * 1024) { return reject("stream_too_large", "Ответ AI слишком большой."); }
    m_buffer += bytes;
    int end;
    while ((end = m_buffer.indexOf('\n')) >= 0) {
        auto line = m_buffer.left(end); m_buffer.remove(0, end + 1);
        if (line.endsWith('\r')) line.chop(1);
        if (line.isEmpty()) { if (!event()) return false; }
        else if (line.startsWith("data:")) {
            auto value = line.mid(5); if (value.startsWith(' ')) value.remove(0, 1);
            m_data += value + '\n';
        } else if (line.startsWith("event:")) m_eventName = line.mid(6).trimmed();
    }
    return true;
}
bool AiResponseStream::event() {
    const auto eventName = m_eventName; m_eventName.clear();
    if (m_data.isEmpty()) return true;
    auto data = m_data; m_data.clear();
    if (data.trimmed() == "[DONE]") {
        if (!m_completed || m_done) { return reject("terminal_incomplete", "AI не завершил ответ."); }
        m_done = true; return true;
    }
    if (m_completed || m_done) { return reject("event_after_completion", "Получен некорректный поток ответа AI."); }
    QJsonParseError parse;
    const auto doc = QJsonDocument::fromJson(data, &parse);
    if (parse.error != QJsonParseError::NoError || !doc.isObject()) { return reject("invalid_event_json", "Не удалось прочитать ответ AI."); }
    const auto object = doc.object();
    const auto type = object.value("type").toString(QString::fromLatin1(eventName));
    if (type.isEmpty()) { return reject("missing_event_type", "Некорректное событие AI."); }
    if (type == "error" || type == "response.failed" || type == "response.incomplete") {
        auto error = object.value("response").toObject().value("error").toObject();
        if (error.isEmpty()) error = object.value("error").toObject();
        if (error.isEmpty()) error = object;
        const auto code = error.value("code").toString();
        const auto parameter = error.value("param").toString();
        const QStringList allowedParameters{"model", "input", "instructions", "store", "stream", "tools", "reasoning", "text", "service_tier"};
        m_parameter = parameter.isEmpty() ? "none" : allowedParameters.contains(parameter) ? parameter : "other";
        // Only known protocol codes enter diagnostics. Remote messages may contain echoed input.
        if (code == "subscription_sharing_usage_limit_exceeded" || code == "rate_limit_exceeded")
            return reject("usage_limit", "Достигнут лимит ChatGPT. Попробуйте позже.");
        if (code == "subscription_sharing_user_not_eligible")
            return reject("user_not_eligible", "Использование тарифа ChatGPT недоступно для выбранного аккаунта или рабочего пространства.");
        if (code == "subscription_sharing_usage_unavailable" || code == "subscription_sharing_user_unavailable" || code == "server_error")
            return reject("service_unavailable", "ChatGPT временно не может обработать запрос. Попробуйте позже.");
        if (code == "subscription_sharing_invalid_user" || code == "invalid_api_key")
            return reject("invalid_session", "ChatGPT отклонил сеанс. Выйдите и войдите снова.");
        if (code == "subscription_sharing_unsupported_capability" || code == "unsupported_parameter")
            return reject("unsupported_capability", "ChatGPT не поддерживает часть запроса этой версии приложения. Требуется исправление подключения.");
        if (code == "chatpass_v2_scope_not_authorized" || code == "chatpass_v2_invalid_authorization_context" || code == "subscription_sharing_route_not_supported")
            return reject("permission_context", "ChatGPT отклонил разрешения подключения. Требуется проверка интеграции.");
        if (code == "model_not_found")
            return reject("model_unavailable", "Выбранная модель сейчас недоступна в ChatGPT.");
        return reject(type == "response.incomplete" ? "server_incomplete" : "server_error", "ChatGPT не смог завершить запрос. Попробуйте позже или уточните запрос.");
    }
    if (type == "response.output_item.added" || type == "response.output_item.done") {
        const int index = boundedIndex(object, "output_index", 128);
        const auto item = object.value("item").toObject();
        if (index < 0 || item.isEmpty()) return reject("invalid_item_event", "Не удалось прочитать часть ответа AI.");
        if (item.value("type") != "reasoning" && (item.value("type") != "message" || item.value("role") != "assistant"))
            return reject("unsupported_output_item", "AI вернул неподдерживаемый ответ.");
        if (item.contains("phase")) m_phases[index] = item.value("phase").toString();
        if (type == "response.output_item.done") {
            if (m_items.contains(index)) return reject("duplicate_output_item", "Получен повторяющийся ответ AI.");
            m_items[index] = item;
        }
        return true;
    }
    if (type == "response.output_text.done") {
        const int index = boundedIndex(object, "output_index", 128);
        const int content = boundedIndex(object, "content_index", 16);
        if (index < 0 || content < 0 || !object.value("text").isString() || m_parts[index].contains(content))
            return reject("invalid_text_event", "Не удалось прочитать текст ответа AI.");
        m_parts[index][content] = object.value("text").toString();
        return true;
    }
    if (type != "response.completed") return true;
    const auto response = object.value("response").toObject();
    if (response.value("status") != "completed" || (response.contains("output") && !response.value("output").isArray()) ||
        (response.contains("error") && !response.value("error").isNull()))
        return reject("terminal_incomplete", "AI не завершил ответ.");
    QMap<int, QJsonObject> assembled = m_items;
    for (auto it = m_parts.begin(); it != m_parts.end(); ++it) {
        if (assembled.contains(it.key())) continue; // Completed items already contain all content parts.
        QJsonArray content;
        int expected = 0;
        for (auto part = it.value().begin(); part != it.value().end(); ++part) {
            if (part.key() != expected++) return reject("missing_text_part", "В ответе AI отсутствует часть текста.");
            content.append(QJsonObject{{"type", "output_text"}, {"text", part.value()}});
        }
        QJsonObject item{{"type", "message"}, {"role", "assistant"}, {"status", "completed"}, {"content", content}};
        if (m_phases.contains(it.key())) item["phase"] = m_phases.value(it.key());
        assembled[it.key()] = item;
    }
    QJsonArray streamed;
    for (const auto &item : assembled) streamed.append(item);
    const auto terminal = selectFinal(response.value("output").toArray());
    const auto collected = selectFinal(streamed);
    m_shape = QString("messages=%1 finals=%2 commentary=%3 streamed_items=%4 text_done=%5")
        .arg(terminal.messages).arg(terminal.finals).arg(terminal.commentary).arg(m_items.size()).arg(m_parts.size());
    if (!terminal.reason.isEmpty() || !collected.reason.isEmpty())
        return reject(!terminal.reason.isEmpty() ? terminal.reason : collected.reason, "Не удалось выделить финальный ответ AI.");
    if (!terminal.text.isEmpty() && !collected.text.isEmpty() && terminal.text != collected.text)
        return reject("conflicting_final_output", "Части ответа AI не совпадают. Повторите запрос.");
    m_text = terminal.text.isEmpty() ? collected.text : terminal.text;
    if (m_text.trimmed().isEmpty()) return reject("missing_final_text", "ChatGPT завершил запрос без финального ответа. Повторите запрос.");
    if (m_text.toUtf8().size() > 128 * 1024) return reject("output_text_too_large", "Ответ AI слишком большой.");
    m_completed = true;
    return true;
}
bool AiResponseStream::finish() {
    if (!m_error.isEmpty()) return false;
    // A truncated event (including terminal JSON without its SSE separator) is not accepted.
    if (!m_completed || !m_buffer.trimmed().isEmpty() || !m_data.trimmed().isEmpty()) {
        return reject("truncated_stream", "Связь с AI прервалась до завершения ответа. Попробуйте ещё раз.");
    }
    return true;
}

bool AiResponseStream::reject(const QString &reason, const QString &message) {
    m_reason = reason; m_error = message; return false;
}
