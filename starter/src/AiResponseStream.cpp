#include "AiResponseStream.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStringList>

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
    if (type != "response.completed") return true;
    const auto response = object.value("response").toObject();
    if (response.value("status") != "completed" || !response.value("output").isArray() || (response.contains("error") && !response.value("error").isNull())) {
        return reject("terminal_incomplete", "AI не завершил ответ.");
    }
    int messages = 0;
    for (const auto &entry : response.value("output").toArray()) {
        const auto item = entry.toObject();
        if (item.value("type") == "reasoning") continue;
        if (item.value("type") != "message" || item.value("role") != "assistant" || (item.contains("status") && item.value("status") != "completed") || !item.value("content").isArray()) {
            return reject("unsupported_output_item", "AI вернул неподдерживаемый ответ.");
        }
        ++messages;
        for (const auto &part : item.value("content").toArray()) {
            const auto content = part.toObject();
            if (content.value("type") != "output_text" || !content.value("text").isString()) { return reject("unsupported_output_content", "AI не подготовил изменение."); }
            m_text += content.value("text").toString();
        }
    }
    if (messages != 1 || m_text.trimmed().isEmpty() || m_text.toUtf8().size() > 128 * 1024) {
        return reject("invalid_output_text", "Не удалось получить изменение от AI.");
    }
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
