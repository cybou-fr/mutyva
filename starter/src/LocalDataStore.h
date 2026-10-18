#pragma once
#include <QMap>
#include <QSqlDatabase>
#include <QString>
#include <QVariantList>

struct LocalDataSnapshot {
    QMap<QString, QString> inputs;
    QMap<QString, QVariantList> lists;
    qint64 revision = 0;
};

// App-scoped user data. Neither configuration history nor AI context owns this store.
class LocalDataStore {
public:
    LocalDataStore();
    ~LocalDataStore();
    LocalDataStore(const LocalDataStore &) = delete;
    LocalDataStore &operator=(const LocalDataStore &) = delete;
    bool open(const QString &path, const QString &appId, LocalDataSnapshot &snapshot, QString &error);
    bool save(LocalDataSnapshot &candidate, QString &error);
private:
    bool load(LocalDataSnapshot &snapshot, QString &error);
    bool fail(QString &error, const QString &message);
    QSqlDatabase m_db;
    QString m_connection;
    QString m_appId;
    LocalDataSnapshot m_current;
};
