#pragma once
#include <QObject>
#include <QVariantMap>

class BrandIdentity : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap colors READ colors CONSTANT)
    Q_PROPERTY(QString iconUrl READ iconUrl CONSTANT)
    Q_PROPERTY(QString logoUrl READ logoUrl CONSTANT)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(bool welcomeRequired READ welcomeRequired NOTIFY changed)
public:
    explicit BrandIdentity(bool persistent = true, QObject *parent = nullptr);
    QVariantMap colors() const;
    QString iconUrl() const { return "qrc:/branding/exports/mutyva-icon.png"; }
    QString logoUrl() const { return "qrc:/branding/exports/mutyva-logo.png"; }
    QString version() const;
    bool welcomeRequired() const { return m_welcomeRequired; }
    Q_INVOKABLE void finishWelcome();
signals:
    void changed();
private:
    bool m_persistent;
    bool m_welcomeRequired;
};
