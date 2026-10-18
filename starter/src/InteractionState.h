#pragma once
#include <QObject>
#include <QMap>
#include <QStringList>
#include <QVariantList>
#include "AppDocument.h"
#include "LocalDataStore.h"
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
private:
    QJsonObject widget(const QString &id) const;
    bool reject(const QString &error);
    bool persist(LocalDataSnapshot &candidate);
    AppDocument *m_document;
    LocalDataSnapshot m_snapshot;
    std::unique_ptr<LocalDataStore> m_store;
    bool m_ready = false;
    int m_revision = 0;
    QString m_error;
signals:
    void changed();
};
