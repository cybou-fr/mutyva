#pragma once
#include <QObject>
#include <QMap>
#include <QStringList>
#include <QVariantList>
#include "AppDocument.h"
#include "LocalDataStore.h"
#include "RecordStore.h"
#include <memory>

// Persistent user data is separate from versioned configuration and AI context.
class InteractionState : public QObject {
    Q_OBJECT
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString lastError READ lastError NOTIFY changed)
    Q_PROPERTY(bool ready READ ready NOTIFY changed)
    Q_PROPERTY(QString storagePath READ storagePath CONSTANT)
public:
    explicit InteractionState(AppDocument *document, QObject *parent = nullptr);
    int revision() const { return m_revision; }
    QString lastError() const { return m_error; }
    bool ready() const { return m_ready; }
    QString storagePath() const { return m_document->userDataPath(); }
    Q_INVOKABLE QString value(const QString &id, const QString &fallback = {}) const;
    Q_INVOKABLE QVariantList items(const QString &id, const QVariantList &fallback = {}) const;
    Q_INVOKABLE bool setValue(const QString &id, const QString &value);
    Q_INVOKABLE bool trigger(const QString &buttonId);
    Q_INVOKABLE bool reload();
    Q_INVOKABLE QVariantList modelFields(const QString &modelId) const;
    Q_INVOKABLE QVariantList records(const QString &widgetId);
    Q_INVOKABLE QVariantMap selectedRecord(const QString &modelId);
    Q_INVOKABLE bool selectRecord(const QString &widgetId, const QString &recordId);
    Q_INVOKABLE bool clearSelectedRecord(const QString &formWidgetId);
    Q_INVOKABLE bool createRecord(const QString &widgetId, const QVariantMap &values);
    Q_INVOKABLE bool updateRecord(const QString &widgetId, const QString &recordId, const QVariantMap &values);
    Q_INVOKABLE bool deleteRecord(const QString &widgetId, const QString &recordId);
private:
    QJsonObject widget(const QString &id) const;
    bool reject(const QString &error);
    bool persist(LocalDataSnapshot &candidate);
    QJsonObject model(const QString &modelId) const;
    bool normalize(const QString &modelId, const QVariantMap &values, QJsonObject &result);
    bool recordError(const QString &message);
    AppDocument *m_document;
    LocalDataSnapshot m_snapshot;
    std::unique_ptr<LocalDataStore> m_store;
    std::unique_ptr<RecordStore> m_records;
    QMap<QString, QString> m_selectedRecords;
    bool m_ready = false;
    int m_revision = 0;
    QString m_error;
signals:
    void changed();
};
