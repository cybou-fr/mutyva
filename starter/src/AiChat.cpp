#include "AiChat.h"
#include "AppDocument.h"
#include "ChatSession.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QTimer>
#include <memory>

AiChat::AiChat(AppDocument *document, ChatSession *session, QObject *parent)
    : QObject(parent), m_document(document), m_session(session) {
    connect(session, &ChatSession::changed, this, [this] {
        if (m_busy && !m_session->connected()) fail("Соединение с ChatGPT завершено. Войдите ещё раз.");
    });
}
void AiChat::append(const QString &role, const QString &text) {
    m_messages.append(QVariantMap{{"role", role}, {"text", text}});
    while (m_messages.size() > 24) m_messages.removeFirst();
    emit changed();
}
void AiChat::cancel() {
    ++m_epoch;
    if (m_reply) { auto reply = m_reply; m_reply.clear(); reply->abort(); }
    const bool wasBusy = m_busy;
    m_busy = false; m_packet = {}; m_stream = AiResponseStream();
    if (wasBusy && !m_messages.isEmpty() && m_messages.last().toMap().value("role") == "user") {
        auto message = m_messages.last().toMap(); message.insert("unfinished", true); m_messages.last() = message;
    }
    m_status = wasBusy ? "Запрос отменён. Приложение не изменено." : QString(); emit changed();
}
void AiChat::fail(const QString &text) { cancel(); m_status = text; emit changed(); }
bool AiChat::send(const QString &text) {
    if (m_busy) return false;
    if (!m_session->connected()) { fail("Сначала войдите через ChatGPT."); return false; }
    if (m_document->hasPreview()) { fail("Сначала подтвердите или отмените предложенное изменение."); return false; }
    if (!m_document->prepareDesignerContext(text)) { fail("Введите запрос от 1 до 4000 символов."); return false; }
    m_packet = QJsonDocument::fromJson(m_document->designerContextJson().toUtf8()).object();
    m_generation = m_document->generation();
    append("user", text.trimmed());
    m_busy = true; m_status = "AI готовит ответ…"; m_stream = AiResponseStream(); emit changed();
    QNetworkRequest request(QUrl("https://api.openai.com/v1/models"));
    request.setRawHeader("Authorization", "Bearer " + m_session->accessToken());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    auto *reply = m_network.get(request); m_reply = reply; reply->setReadBufferSize(64 * 1024);
    QTimer::singleShot(30000, reply, [reply] { if (!reply->isFinished()) reply->abort(); });
    const auto epoch = ++m_epoch;
    auto bytes = std::make_shared<QByteArray>();
    connect(reply, &QNetworkReply::readyRead, this, [reply, bytes] { *bytes += reply->readAll(); if (bytes->size() > 512 * 1024) reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, bytes, epoch] {
        reply->deleteLater(); if (epoch != m_epoch) return;
        m_reply.clear(); *bytes += reply->readAll();
        const auto model = AiResponseStream::selectModel(QJsonDocument::fromJson(*bytes).object());
        if (reply->error() != QNetworkReply::NoError || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200 || bytes->size() > 512 * 1024 || model.isEmpty()) {
            fail("Не удалось получить доступ к AI. Проверьте сеть, разрешения и лимиты ChatGPT."); return;
        }
        this->request(model);
    });
    return true;
}
void AiChat::request(const QString &model) {
    if (!m_session->connected()) { fail("Войдите в ChatGPT ещё раз."); return; }
    QJsonArray input;
    // Recent conversation helps resolve clarifications; the saved document is always rebuilt independently.
    for (const auto &entry : m_messages) {
        const auto message = entry.toMap();
        if (message.value("unfinished").toBool()) continue;
        input.append(QJsonObject{{"role", message.value("role").toString()}, {"content", message.value("text").toString()}});
    }
    input.append(QJsonObject{{"role", "user"}, {"content", QString::fromUtf8(QJsonDocument(m_packet.value("context").toObject()).toJson(QJsonDocument::Compact))}});
    const QJsonObject body{{"model", model}, {"store", false}, {"stream", true},
        {"instructions", m_packet.value("instructions")}, {"input", input}};
    const auto bytes = QJsonDocument(body).toJson(QJsonDocument::Compact);
    if (bytes.size() > 256 * 1024) { fail("Запрос слишком большой. Начните с более короткого описания."); return; }
    QNetworkRequest request(QUrl("https://api.openai.com/v1/responses"));
    request.setRawHeader("Authorization", "Bearer " + m_session->accessToken());
    request.setRawHeader("Accept", "text/event-stream");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(60000);
    auto *reply = m_network.post(request, bytes); m_reply = reply; reply->setReadBufferSize(64 * 1024);
    const auto epoch = m_epoch;
    QTimer::singleShot(180000, reply, [reply] { if (!reply->isFinished()) reply->abort(); });
    connect(reply, &QNetworkReply::readyRead, this, [this, reply, epoch] {
        if (epoch != m_epoch) return;
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status != 200) { reply->readAll(); return; }
        if (!m_stream.feed(reply->readAll())) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, epoch] {
        reply->deleteLater(); if (epoch != m_epoch) return;
        m_reply.clear();
        const auto contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        if (reply->error() != QNetworkReply::NoError || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200 || !contentType.startsWith("text/event-stream")) {
            fail(m_stream.error().isEmpty() ? "Не удалось завершить запрос к AI. Проверьте сеть и доступность ChatGPT." : m_stream.error()); return;
        }
        if (!m_stream.feed(reply->readAll()) || !m_stream.finish()) { fail(m_stream.error()); return; }
        const auto text = m_stream.text(); const int generation = m_generation;
        m_packet = {}; m_stream = AiResponseStream();
        acceptResponse(text, generation);
    });
}
bool AiChat::acceptResponse(const QString &text, int baseGeneration) {
    if (m_document->generation() != baseGeneration || m_document->hasPreview()) {
        fail("Приложение уже изменилось. Отправьте запрос ещё раз."); return false;
    }
    if (!m_document->previewDesignerResponse(text)) { fail("AI предложил изменение, которое пока нельзя применить. Попробуйте уточнить запрос."); return false; }
    auto message = m_document->designerMessage();
    if (message.isEmpty()) message = "Изменение готово. Посмотрите предпросмотр и подтвердите его.";
    m_busy = false;
    append("assistant", message);
    m_status = m_document->hasPreview() ? "Предпросмотр готов. Примените или отмените изменение." : QString();
    emit changed();
    if (m_document->hasPreview()) emit proposalReady();
    return true;
}
