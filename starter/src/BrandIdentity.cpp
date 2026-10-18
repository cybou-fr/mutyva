#include "BrandIdentity.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>

BrandIdentity::BrandIdentity(bool persistent, QObject *parent)
    : QObject(parent), m_persistent(persistent),
      m_welcomeRequired(persistent && !QSettings().value("branding/welcomeSeen", false).toBool()) {}

QVariantMap BrandIdentity::colors() const {
    QFile file(":/branding/palette.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object().toVariantMap();
}

QString BrandIdentity::version() const {
    return QCoreApplication::applicationVersion().isEmpty() ? "dev" : QCoreApplication::applicationVersion();
}

void BrandIdentity::finishWelcome() {
    if (m_persistent) QSettings().setValue("branding/welcomeSeen", true);
    m_welcomeRequired = false;
    emit changed();
}
