#include "AiChat.h"
#include "AppDocument.h"
#include "ChatSession.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QTimer>
#include <QDebug>
#include <memory>

AiChat::AiChat(AppDocument *document, ChatSession *session, QObject *parent, QNetworkAccessManager *network)
    : QObject(parent), m_document(document), m_session(session), m_network(network ? network : &m_ownedNetwork) {
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
    m_diagnostics.clear();
    m_packet = QJsonDocument::fromJson(m_document->designerContextJson().toUtf8()).object();
    m_generation = m_document->generation();
    append("user", text.trimmed());
    m_busy = true; m_status = "AI готовит ответ…"; m_stream = AiResponseStream(); emit changed();
    QNetworkRequest request(QUrl("https://api.openai.com/v1/models"));
    request.setRawHeader("Authorization", "Bearer " + m_session->accessToken());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    auto *reply = m_network->get(request); m_reply = reply; reply->setReadBufferSize(64 * 1024);
    QTimer::singleShot(30000, reply, [reply] { if (!reply->isFinished()) reply->abort(); });
    const auto epoch = ++m_epoch;
    auto bytes = std::make_shared<QByteArray>();
    connect(reply, &QNetworkReply::readyRead, this, [reply, bytes] { *bytes += reply->readAll(); if (bytes->size() > 4 * 1024 * 1024) reply->abort(); });
    connect(reply, &QNetworkReply::finished, this, [this, reply, bytes, epoch] {
        reply->deleteLater(); if (epoch != m_epoch) return;
        m_reply.clear(); if (reply->isOpen()) *bytes += reply->readAll();
        if (bytes->size() > 4 * 1024 * 1024) { serviceFailure("models", reply, bytes->size(), "catalog_too_large"); return; }
        if (reply->error() != QNetworkReply::NoError || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
            serviceFailure("models", reply, bytes->size(), "http_or_network"); return;
        }
        QJsonParseError parse;
        const auto catalog = QJsonDocument::fromJson(*bytes, &parse);
        if (parse.error != QJsonParseError::NoError || !catalog.isObject()) {
            serviceFailure("models", reply, bytes->size(), "invalid_json"); return;
        }
        const auto object = catalog.object();
        if (!object.value("models").isArray()) {
            serviceFailure("models", reply, bytes->size(), object.value("data").isArray() ? "api_catalog_instead_of_chatgpt" : "missing_models"); return;
        }
        const auto model = AiResponseStream::selectModel(object);
        if (model.isEmpty()) {
            serviceFailure("models", reply, bytes->size(), object.value("models").toArray().isEmpty() ? "empty_catalog" : "no_visible_model"); return;
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
        QJsonObject inputMessage{{"role", message.value("role").toString()}, {"content", message.value("text").toString()}};
        if (message.value("role") == "assistant") inputMessage["phase"] = "final_answer";
        input.append(inputMessage);
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
    auto *reply = m_network->post(request, bytes); m_reply = reply; reply->setReadBufferSize(64 * 1024);
    const auto epoch = m_epoch;
    QTimer::singleShot(180000, reply, [reply] { if (!reply->isFinished()) { reply->setProperty("aiDeadline", true); reply->abort(); } });
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
            serviceFailure("responses", reply, m_stream.bytesReceived(), !m_stream.reason().isEmpty() ? m_stream.reason() : reply->property("aiDeadline").toBool() ? "deadline" : "http_or_stream"); return;
        }
        if (!m_stream.feed(reply->isOpen() ? reply->readAll() : QByteArray()) || !m_stream.finish()) {
            serviceFailure("responses", reply, m_stream.bytesReceived(), m_stream.reason().isEmpty() ? "incomplete_stream" : m_stream.reason()); return;
        }
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

void AiChat::serviceFailure(const QString &stage, QNetworkReply *reply, qint64 bytes, const QString &reason) {
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const int network = int(reply->error());
    // Fixed stage/reason enums and numeric metadata only: never log bodies, headers, URLs, tokens or prompts.
    m_diagnostics = QString("AI stage=%1 http=%2 network=%3 bytes=%4 reason=%5")
        .arg(stage).arg(http).arg(network).arg(bytes).arg(reason);
    if (stage == "responses" && !m_stream.errorParameter().isEmpty()) m_diagnostics += " param=" + m_stream.errorParameter();
    if (stage == "responses" && !m_stream.outputShape().isEmpty()) m_diagnostics += " " + m_stream.outputShape();
    qInfo().noquote() << m_diagnostics;
    QString message;
    if (stage == "responses" && !m_stream.error().isEmpty()) message = m_stream.error();
    else if (reason == "deadline") message = "ChatGPT не ответил за отведённое время. Попробуйте ещё раз.";
    else if (http == 401) message = "ChatGPT отклонил сеанс. Выйдите и войдите снова.";
    else if (http == 403) message = "ChatGPT не разрешил доступ к AI. Проверьте разрешение использовать ваш тариф в настройках ChatGPT.";
    else if (http == 429) message = "Достигнут лимит ChatGPT. Попробуйте позже.";
    else if (http >= 500) message = "Сервис ChatGPT временно недоступен. Попробуйте позже.";
    else if (http >= 300 && http < 400) message = "Адрес сервиса ChatGPT изменился. Требуется обновление подключения.";
    else if (reason == "catalog_too_large") message = "Каталог ChatGPT слишком большой для этой версии приложения.";
    else if (network != QNetworkReply::NoError) message = "Не удалось связаться с ChatGPT. Проверьте подключение и повторите запрос.";
    else if (reason == "empty_catalog" || reason == "no_visible_model") message = "ChatGPT не предоставил доступную модель для этого аккаунта.";
    else if (stage == "models") message = "ChatGPT вернул неподдерживаемый каталог моделей. Подключение требует проверки.";
    else message = m_stream.error().isEmpty() ? "ChatGPT не завершил ответ. Повторите запрос." : m_stream.error();
    fail(message);
}
