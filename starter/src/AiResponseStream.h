#pragma once
#include <QByteArray>
#include <QString>
#include <QJsonObject>
#include <QMap>
#include <QJsonArray>

// Bounded SSE reader: only the terminal completed response may become a proposal.
class AiResponseStream {
public:
    bool feed(const QByteArray &bytes);
    bool finish();
    QString text() const { return m_text; }
    QString error() const { return m_error; }
    QString reason() const { return m_reason; }
    QString errorParameter() const { return m_parameter; }
    QString outputShape() const { return m_shape; }
    qint64 bytesReceived() const { return m_total; }
    static QString selectModel(const QJsonObject &catalog);
private:
    bool event();
    bool reject(const QString &reason, const QString &message);
    QMap<int, QJsonObject> m_items;
    QMap<int, QMap<int, QString>> m_parts;
    QMap<int, QString> m_phases;
    QString m_shape;
    QByteArray m_buffer, m_data;
    QString m_text, m_error, m_reason, m_parameter;
    QByteArray m_eventName;
    qint64 m_total = 0;
    bool m_completed = false, m_done = false;
};
