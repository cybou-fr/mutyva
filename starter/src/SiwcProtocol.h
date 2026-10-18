#pragma once
#include <QJsonObject>
#include <QUrl>

namespace SiwcProtocol {
QString randomSecret();
QString challenge(const QString &verifier);
bool callback(const QUrl &url, const QString &state, const QString &registeredClient,
              QString &code, QString &client);
bool verifyIdentity(const QString &jwt, const QJsonObject &jwks, const QString &client,
                    const QString &nonce, qint64 now, QJsonObject &identity);
}
