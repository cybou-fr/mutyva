#pragma once
#include <QObject>
#include <QVariantList>
#include <QTemporaryDir>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
class AppDocument;
class AiChat;
class ChatSession;
class QNetworkAccessManager;

// Human sign-in, then real production ChatSession/AiChat calls against an isolated app.
class LiveAiCheck : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QVariantList results READ results NOTIFY changed)
    Q_PROPERTY(bool finished READ finished NOTIFY changed)
public:
    LiveAiCheck(ChatSession *session, const QString &reportPath, QObject *parent = nullptr,
                QNetworkAccessManager *network = nullptr);
    ~LiveAiCheck() override;
    QString status() const { return m_status; }
    QVariantList results() const { return m_results; }
    bool finished() const { return m_finished; }
    Q_INVOKABLE void start();
    Q_INVOKABLE void cancel();
signals:
    void changed();
    void completed(int exitCode);
private:
    void nextCase();
    void checkReply();
    bool publish(const QString &status, const QString &reason, int exitCode);
    void finish(const QString &status, const QString &reason, int exitCode);
    void record(const QString &name, bool passed, const QString &reason, const QString &reply = {});
    ChatSession *m_session;
    QNetworkAccessManager *m_network;
    QString m_reportPath, m_status = "Войдите через ChatGPT. После входа проверка выполнится автоматически.";
    QTemporaryDir m_store;
    std::unique_ptr<AppDocument> m_document;
    std::unique_ptr<AiChat> m_chat;
    QVariantList m_results;
    QTimer m_deadline;
    QElapsedTimer m_elapsed;
    int m_case = -1;
    bool m_started = false, m_finished = false, m_inFlight = false, m_authBusy = false;
};
