#pragma once
#include <QByteArray>
#include <QString>
#include <QJsonObject>

// Bounded SSE reader: only the terminal completed response may become a proposal.
class AiResponseStream {
public:
    bool feed(const QByteArray &bytes);
    bool finish();
    QString text() const { return m_text; }
    QString error() const { return m_error; }
    static QString selectModel(const QJsonObject &catalog);
private:
    bool event();
    QByteArray m_buffer, m_data;
    QString m_text, m_error;
    qint64 m_total = 0;
    bool m_completed = false, m_done = false;
};
