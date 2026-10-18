#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTcpServer>
#include <QTimer>
#include <functional>

// Credentials are deliberately C++-only and memory-only in this first integration.
class ChatSession : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
public:
    explicit ChatSession(QObject *parent = nullptr);
    bool connected() const;
    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    QByteArray accessToken() const;
    Q_INVOKABLE void signIn();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void signOut();
signals:
    void changed();
private:
    friend class RuntimeTest;
    void fail(const QString &status);
    void fetch(const QUrl &url, const QByteArray &body, std::function<void(const QJsonObject &)> done, std::function<void()> onError = {});
    void authorize();
    void exchange(const QString &code, const QString &client);
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QTcpServer m_listener;
    QTimer m_timeout, m_expiry;
    quint64 m_epoch = 0;
    bool m_busy = false;
    QString m_status = "Войдите через ChatGPT, чтобы изменить приложение словами.";
    QString m_state, m_nonce, m_verifier, m_redirect, m_client;
    QString m_savedClient, m_savedSubject;
    QByteArray m_access, m_refresh;
    qint64 m_expires = 0;
};
