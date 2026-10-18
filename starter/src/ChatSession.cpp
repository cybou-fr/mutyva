#include "ChatSession.h"
#include "SiwcProtocol.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QSettings>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QUuid>
#include <memory>

namespace {
const QString Auth = "https://auth.openai.com";
const QString Resource = "https://api.openai.com/v1";
QByteArray form(const QList<QPair<QString, QString>> &fields) {
    QByteArray encoded;
    for (const auto &field : fields) {
        if (!encoded.isEmpty()) encoded += '&';
        encoded += QUrl::toPercentEncoding(field.first) + '=' + QUrl::toPercentEncoding(field.second);
    }
    return encoded;
}
}
ChatSession::ChatSession(QObject *parent) : QObject(parent) {
    m_timeout.setSingleShot(true);
    m_expiry.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, [this] { fail("Время ожидания входа истекло. Попробуйте ещё раз."); });
    connect(&m_expiry, &QTimer::timeout, this, [this] {
        m_access.clear(); m_refresh.clear(); m_expires = 0;
        m_status = "Сеанс завершён. Войдите через ChatGPT ещё раз."; emit changed();
    });
    connect(&m_listener, &QTcpServer::newConnection, this, [this] {
        while (m_listener.hasPendingConnections()) {
            auto *socket = m_listener.nextPendingConnection();
            socket->setReadBufferSize(16384);
            const auto epoch = m_epoch;
            auto buffer = std::make_shared<QByteArray>();
            QTimer::singleShot(5000, socket, [socket] { socket->abort(); socket->deleteLater(); });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer, epoch] {
                if (socket->property("handled").toBool() || epoch != m_epoch || !m_busy) { socket->disconnectFromHost(); return; }
                *buffer += socket->readAll();
                if (buffer->size() > 16384) { socket->abort(); return; }
                if (!buffer->contains("\r\n\r\n")) return;
                const auto lines = buffer->split('\n');
                const auto first = lines.first().trimmed().split(' ');
                const QByteArray expectedHost = "host: 127.0.0.1:" + QByteArray::number(m_listener.serverPort());
                bool host = false;
                for (const auto &line : lines) if (line.trimmed().toLower() == expectedHost) host = true;
                const auto target = first.size() == 3 ? QUrl::fromEncoded(first[1], QUrl::StrictMode) : QUrl();
                if (first.size() != 3 || first[0] != "GET" || !host || target.isRelative() == false || target.path() != "/auth/callback") {
                    socket->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
                    socket->disconnectFromHost(); return;
                }
                socket->setProperty("handled", true);
                QString code, client;
                const bool valid = SiwcProtocol::callback(target, m_state, m_savedClient, code, client);
                const QByteArray body = valid ? "Return to MUTYVA to finish signing in." : "Sign-in rejected. Return to MUTYVA and try again.";
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\nContent-Security-Policy: default-src 'none'\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
                m_listener.close(); m_state.clear();
                if (!valid) { fail("Вход отменён или ответ не прошёл проверку. Попробуйте ещё раз."); return; }
                exchange(code, client);
            });
        }
    });
}
bool ChatSession::connected() const { return !m_access.isEmpty() && m_expires > QDateTime::currentSecsSinceEpoch() + 5; }
QByteArray ChatSession::accessToken() const { return connected() ? m_access : QByteArray(); }
void ChatSession::cancel() {
    ++m_epoch; m_listener.close(); m_timeout.stop(); m_busy = false;
    if (m_reply) { auto reply = m_reply; m_reply.clear(); reply->abort(); }
    m_state.clear(); m_nonce.clear(); m_verifier.clear();
    m_status = connected() ? "ChatGPT подключён." : "Вход отменён. Можно продолжать пользоваться приложением.";
    emit changed();
}
void ChatSession::fail(const QString &status) { cancel(); m_status = status; emit changed(); }
void ChatSession::fetch(const QUrl &url, const QByteArray &body, std::function<void(const QJsonObject &)> done, std::function<void()> onError) {
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    auto *reply = body.isEmpty() ? m_network.get(request) : m_network.post(request, body);
    reply->setReadBufferSize(512 * 1024);
    m_reply = reply;
    const auto epoch = m_epoch;
    auto bytes = std::make_shared<QByteArray>();
    connect(reply, &QNetworkReply::readyRead, this, [reply, bytes] {
        *bytes += reply->readAll();
        if (bytes->size() > 512 * 1024) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, bytes, epoch, done, onError] {
        reply->deleteLater();
        if (epoch != m_epoch) return;
        m_reply.clear(); *bytes += reply->readAll();
        QJsonParseError error;
        const auto json = QJsonDocument::fromJson(*bytes, &error);
        if (reply->error() != QNetworkReply::NoError || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200 ||
            bytes->size() > 512 * 1024 || error.error != QJsonParseError::NoError || !json.isObject()) {
            if (onError) { onError(); return; }
            fail("Не удалось завершить подключение к ChatGPT. Проверьте сеть и повторите вход."); return;
        }
        done(json.object());
    });
}
void ChatSession::signIn() {
#ifndef Q_OS_WIN
    fail("Вход в этой сборке пока доступен только в Windows."); return;
#endif
    if (m_busy || connected()) return;
    cancel(); m_busy = true; m_status = "Подключаемся к ChatGPT…"; emit changed();
    m_timeout.start(10 * 60 * 1000);
    fetch(QUrl(Auth + "/.well-known/openid-configuration"), {}, [this](const QJsonObject &metadata) {
        // Fail closed if production discovery changes. Never send credentials to an arbitrary discovered host.
        if (metadata.value("issuer") != Auth || metadata.value("authorization_endpoint") != Auth + "/api/accounts/authorize" ||
            metadata.value("token_endpoint") != Auth + "/api/accounts/oauth/token" ||
            metadata.value("jwks_uri") != Auth + "/.well-known/jwks.json") {
            fail("Параметры входа ChatGPT изменились. Требуется обновление приложения."); return;
        }
        authorize();
    });
}
void ChatSession::authorize() {
    QSettings settings;
    auto host = settings.value("chatgpt/hostId").toString();
    if (host.isEmpty()) {
        host = "urn:uuid:" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        settings.setValue("chatgpt/hostId", host); settings.sync();
    }
    if (settings.status() != QSettings::NoError) { fail("Не удалось сохранить настройки входа."); return; }
    m_savedClient = settings.value("chatgpt/clientId").toString();
    m_savedSubject = settings.value("chatgpt/subject").toString();
    if (!m_listener.listen(QHostAddress::LocalHost, 0)) { fail("Не удалось начать вход. Попробуйте ещё раз."); return; }
    m_redirect = "http://127.0.0.1:" + QString::number(m_listener.serverPort()) + "/auth/callback";
    m_state = SiwcProtocol::randomSecret(); m_nonce = SiwcProtocol::randomSecret(); m_verifier = SiwcProtocol::randomSecret();
    QUrl url(Auth + "/api/accounts/authorize");
    QUrlQuery query;
    const QList<QPair<QString,QString>> fields{
        {"client_id", m_savedClient.isEmpty() ? "dynamic_agent_client" : m_savedClient},
        {"agent_name_hint", "MUTYVA"}, {"ext_agent_host_id", host}, {"response_type", "code"},
        {"redirect_uri", m_redirect}, {"resource", Resource},
        {"scope", "openid profile email offline_access resource.invoke chatgpt.tokens.use.direct"},
        {"state", m_state}, {"nonce", m_nonce}, {"code_challenge_method", "S256"},
        {"code_challenge", SiwcProtocol::challenge(m_verifier)}};
    for (const auto &field : fields) query.addQueryItem(field.first, field.second);
    url.setQuery(query);
    m_status = "Завершите вход в браузере, затем вернитесь в MUTYVA."; emit changed();
    if (!QDesktopServices::openUrl(url)) fail("Не удалось открыть браузер для входа.");
}
void ChatSession::exchange(const QString &code, const QString &client) {
    m_client = client;
    const auto body = form({{"grant_type", "authorization_code"}, {"client_id", client}, {"code", code},
        {"code_verifier", m_verifier}, {"redirect_uri", m_redirect}, {"resource", Resource}});
    m_verifier.clear();
    m_status = "Проверяем подключение…"; emit changed();
    fetch(QUrl(Auth + "/api/accounts/oauth/token"), body, [this](const QJsonObject &tokens) {
        const auto scopes = tokens.value("scope").toString().split(' ', Qt::SkipEmptyParts);
        const auto lifetime = tokens.value("expires_in").toInt();
        if (!scopes.contains("chatgpt.tokens.use.direct") || tokens.value("token_type").toString().compare("Bearer", Qt::CaseInsensitive) ||
            tokens.value("access_token").toString().isEmpty() || tokens.value("id_token").toString().isEmpty() || lifetime <= 30 || lifetime > 86400) {
            fail("ChatGPT не разрешил использовать AI в приложении. Проверьте разрешения при входе."); return;
        }
        fetch(QUrl(Auth + "/.well-known/jwks.json"), {}, [this, tokens, lifetime](const QJsonObject &keys) {
            QJsonObject identity;
            if (!SiwcProtocol::verifyIdentity(tokens.value("id_token").toString(), keys, m_client, m_nonce,
                QDateTime::currentSecsSinceEpoch(), identity) || (!m_savedSubject.isEmpty() && identity.value("sub").toString() != m_savedSubject)) {
                fail("Не удалось подтвердить учётную запись ChatGPT. Вход отклонён."); return;
            }
            QSettings settings;
            settings.setValue("chatgpt/clientId", m_client); settings.setValue("chatgpt/subject", identity.value("sub").toString()); settings.sync();
            if (settings.status() != QSettings::NoError) { fail("Не удалось сохранить настройки подключения."); return; }
            m_access = tokens.value("access_token").toString().toUtf8();
            m_refresh = tokens.value("refresh_token").toString().toUtf8();
            m_expires = QDateTime::currentSecsSinceEpoch() + lifetime;
            m_expiry.start((lifetime - 10) * 1000);
            m_timeout.stop(); m_nonce.clear(); m_busy = false;
            m_status = "ChatGPT подключён. Опишите, что хотите изменить."; emit changed();
        });
    });
}
void ChatSession::signOut() {
    const auto token = m_refresh.isEmpty() ? m_access : m_refresh;
    const QString hint = m_refresh.isEmpty() ? "access_token" : "refresh_token";
    cancel(); m_access.clear(); m_refresh.clear(); m_expires = 0; m_expiry.stop();
    m_status = "Вы вышли из ChatGPT на этом устройстве."; emit changed();
    if (token.isEmpty()) return;
    m_busy = true; emit changed();
    fetch(QUrl(Auth + "/.well-known/openid-configuration"), {}, [this, token, hint](const QJsonObject &metadata) {
        const auto endpoint = QUrl(metadata.value("revocation_endpoint").toString());
        if (endpoint.scheme() != "https" || endpoint.host() != "auth.openai.com" || !endpoint.userInfo().isEmpty() || endpoint.port(-1) != -1) {
            fail("Вы вышли локально. Отключение доступа можно завершить в настройках ChatGPT."); return;
        }
        QNetworkRequest request(endpoint);
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
        request.setTransferTimeout(15000);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        auto *reply = m_network.post(request, form({{"token", QString::fromUtf8(token)}, {"token_type_hint", hint}, {"client_id", m_client}}));
        m_reply = reply; const auto epoch = m_epoch;
        connect(reply, &QNetworkReply::finished, this, [this, reply, epoch] {
            reply->deleteLater(); if (epoch != m_epoch) return;
            m_reply.clear(); m_busy = false;
            m_status = reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200
                ? "Вы вышли из ChatGPT." : "Вы вышли локально. Отключение доступа можно завершить в настройках ChatGPT.";
            emit changed();
        });
        auto discarded = std::make_shared<qint64>(0);
        reply->setReadBufferSize(64 * 1024);
        connect(reply, &QNetworkReply::readyRead, this, [reply, discarded] {
            *discarded += reply->readAll().size();
            if (*discarded > 64 * 1024) reply->abort();
        });
    }, [this] { fail("Вы вышли локально. Отключение доступа можно завершить в настройках ChatGPT."); });
}
