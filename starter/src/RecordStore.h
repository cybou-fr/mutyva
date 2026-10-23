#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QString>

// Separate, app-scoped relational records. Shares the existing SQLite file but never
// changes widget_data or the legacy PRAGMA user_version=1 storage contract.
class RecordStore final {
public:
    RecordStore();
    ~RecordStore();
    RecordStore(const RecordStore &) = delete;
    RecordStore &operator=(const RecordStore &) = delete;
    bool open(const QString &path, const QString &appId, QString &error);
    bool list(const QString &modelId, QJsonArray &result, QString &error);
    bool create(const QString &modelId, const QJsonObject &values, QString &recordId, QString &error);
    bool update(const QString &modelId, const QString &recordId, const QJsonObject &values, QString &error);
    bool remove(const QString &modelId, const QString &recordId, QString &error);
private:
    bool change(const QString &op, const QString &modelId, const QString &recordId,
                const QJsonObject &values, QString &error);
    QSqlDatabase m_db;
    QString m_connection;
    QString m_appId;
    qint64 m_revision = 0;
};
