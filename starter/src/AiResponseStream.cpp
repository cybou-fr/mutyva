#include "AiResponseStream.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

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
    if (m_total > 2 * 1024 * 1024) { m_error = "Ответ AI слишком большой."; return false; }
    m_buffer += bytes;
    int end;
    while ((end = m_buffer.indexOf('\n')) >= 0) {
        auto line = m_buffer.left(end); m_buffer.remove(0, end + 1);
        if (line.endsWith('\r')) line.chop(1);
        if (line.isEmpty()) { if (!event()) return false; }
        else if (line.startsWith("data:")) {
            auto value = line.mid(5); if (value.startsWith(' ')) value.remove(0, 1);
            m_data += value + '\n';
        }
    }
    return true;
}
bool AiResponseStream::event() {
    if (m_data.isEmpty()) return true;
    auto data = m_data; m_data.clear();
    if (data.trimmed() == "[DONE]") {
        if (!m_completed || m_done) { m_error = "AI не завершил ответ."; return false; }
        m_done = true; return true;
    }
    if (m_completed || m_done) { m_error = "Получен некорректный поток ответа AI."; return false; }
    QJsonParseError parse;
    const auto doc = QJsonDocument::fromJson(data, &parse);
    if (parse.error != QJsonParseError::NoError || !doc.isObject()) { m_error = "Не удалось прочитать ответ AI."; return false; }
    const auto object = doc.object();
    const auto type = object.value("type").toString();
    if (type.isEmpty()) { m_error = "Некорректное событие AI."; return false; }
    if (type == "error" || type == "response.failed" || type == "response.incomplete") {
        m_error = "AI не смог завершить запрос. Проверьте доступность и лимиты ChatGPT и повторите попытку."; return false;
    }
    if (type != "response.completed") return true;
    const auto response = object.value("response").toObject();
    if (response.value("status") != "completed" || !response.value("output").isArray() || (response.contains("error") && !response.value("error").isNull())) {
        m_error = "AI не завершил ответ."; return false;
    }
    int messages = 0;
    for (const auto &entry : response.value("output").toArray()) {
        const auto item = entry.toObject();
        if (item.value("type") == "reasoning") continue;
        if (item.value("type") != "message" || item.value("role") != "assistant" || item.value("status") != "completed" || !item.value("content").isArray()) {
            m_error = "AI вернул неподдерживаемый ответ."; return false;
        }
        ++messages;
        for (const auto &part : item.value("content").toArray()) {
            const auto content = part.toObject();
            if (content.value("type") != "output_text" || !content.value("text").isString()) { m_error = "AI не подготовил изменение."; return false; }
            m_text += content.value("text").toString();
        }
    }
    if (messages != 1 || m_text.trimmed().isEmpty() || m_text.toUtf8().size() > 128 * 1024) {
        m_error = "Не удалось получить изменение от AI."; return false;
    }
    m_completed = true;
    return true;
}
bool AiResponseStream::finish() {
    if (!m_error.isEmpty()) return false;
    // A truncated event (including terminal JSON without its SSE separator) is not accepted.
    if (!m_completed || !m_buffer.trimmed().isEmpty() || !m_data.trimmed().isEmpty()) {
        m_error = "Связь с AI прервалась до завершения ответа. Попробуйте ещё раз."; return false;
    }
    return true;
}
