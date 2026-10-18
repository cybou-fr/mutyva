#pragma once
#include <QObject>
#include <QMap>
#include <QStringList>
#include <QVariantList>
#include "AppDocument.h"

// Session data is separate from versioned configuration and from AI context.
class InteractionState : public QObject {
    Q_OBJECT
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QString lastError READ lastError NOTIFY changed)
public:
    explicit InteractionState(AppDocument *document, QObject *parent = nullptr);
    int revision() const { return m_revision; }
    QString lastError() const { return m_error; }
    Q_INVOKABLE QString value(const QString &id, const QString &fallback = {}) const;
    Q_INVOKABLE QVariantList items(const QString &id, const QVariantList &fallback = {}) const;
    Q_INVOKABLE bool setValue(const QString &id, const QString &value);
    Q_INVOKABLE bool trigger(const QString &buttonId);
private:
    QJsonObject widget(const QString &id) const;
    bool reject(const QString &error);
    void reconcile();
    AppDocument *m_document;
    QMap<QString, QString> m_inputs;
    QMap<QString, QVariantList> m_lists;
    int m_revision = 0;
    QString m_error;
signals:
    void changed();
};
