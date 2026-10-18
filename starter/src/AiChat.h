#pragma once
#include <QObject>
#include <QVariantList>
#include <QNetworkAccessManager>
#include <QPointer>
#include "AiResponseStream.h"
class AppDocument;
class ChatSession;
class AiChat : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList messages READ messages NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    AiChat(AppDocument *document, ChatSession *session, QObject *parent = nullptr);
    QVariantList messages() const { return m_messages; }
    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    Q_INVOKABLE bool send(const QString &text);
    Q_INVOKABLE void cancel();
    // C++ completion boundary, also exercised by offline tests. Never exposed to QML.
    bool acceptResponse(const QString &text, int baseGeneration);
signals:
    void changed();
    void proposalReady();
private:
    void append(const QString &role, const QString &text);
    void fail(const QString &text);
    void request(const QString &model);
    AppDocument *m_document;
    ChatSession *m_session;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QVariantList m_messages;
    QJsonObject m_packet;
    AiResponseStream m_stream;
    int m_generation = 0;
    quint64 m_epoch = 0;
    bool m_busy = false;
    QString m_status;
};
