#include "SiwcProtocol.h"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QUrlQuery>
#include <QRegularExpression>
#include <cstring>
#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#endif

namespace {
QByteArray decode(const QByteArray &s) {
    static const QRegularExpression pattern("\\A[A-Za-z0-9_-]+\\z");
    if (!pattern.match(QString::fromLatin1(s)).hasMatch()) return {};
    auto bytes = QByteArray::fromBase64(s, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    return bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals) == s ? bytes : QByteArray();
}
bool signature(const QByteArray &message, const QByteArray &sig, const QJsonObject &key) {
#ifdef Q_OS_WIN
    const auto n = decode(key.value("n").toString().toLatin1());
    const auto e = decode(key.value("e").toString().toLatin1());
    if (n.size() < 256 || n.size() > 512 || e.isEmpty() || e.size() > 4 || sig.size() != n.size()) return false;
    BCRYPT_RSAKEY_BLOB header{BCRYPT_RSAPUBLIC_MAGIC, ULONG(n.size() * 8), ULONG(e.size()), ULONG(n.size()), 0, 0};
    QByteArray blob(reinterpret_cast<const char *>(&header), sizeof(header)); blob += e; blob += n;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_KEY_HANDLE handle = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_RSA_ALGORITHM, nullptr, 0) < 0) return false;
    bool ok = BCryptImportKeyPair(algorithm, nullptr, BCRYPT_RSAPUBLIC_BLOB, &handle,
        reinterpret_cast<PUCHAR>(blob.data()), ULONG(blob.size()), 0) >= 0;
    if (ok) {
        auto digest = QCryptographicHash::hash(message, QCryptographicHash::Sha256);
        BCRYPT_PKCS1_PADDING_INFO padding{BCRYPT_SHA256_ALGORITHM};
        ok = BCryptVerifySignature(handle, &padding, reinterpret_cast<PUCHAR>(digest.data()), ULONG(digest.size()),
            reinterpret_cast<PUCHAR>(const_cast<char *>(sig.constData())), ULONG(sig.size()), BCRYPT_PAD_PKCS1) >= 0;
    }
    if (handle) BCryptDestroyKey(handle);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return ok;
#else
    Q_UNUSED(message); Q_UNUSED(sig); Q_UNUSED(key);
    return false;
#endif
}
}

QString SiwcProtocol::randomSecret() {
    QByteArray bytes(32, Qt::Uninitialized);
    for (int i = 0; i < bytes.size(); i += 4) {
        const quint32 value = QRandomGenerator::system()->generate();
        std::memcpy(bytes.data() + i, &value, 4);
    }
    return QString::fromLatin1(bytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}
QString SiwcProtocol::challenge(const QString &verifier) {
    return QString::fromLatin1(QCryptographicHash::hash(verifier.toLatin1(), QCryptographicHash::Sha256)
        .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}
bool SiwcProtocol::callback(const QUrl &url, const QString &state, const QString &registeredClient,
                            QString &code, QString &client) {
    code.clear(); client.clear();
    if (state.isEmpty() || !url.isValid() || url.path() != "/auth/callback" || url.hasFragment()) return false;
    QUrlQuery query(url);
    QMap<QString, QString> values;
    for (const auto &item : query.queryItems(QUrl::FullyDecoded)) {
        if (values.contains(item.first)) return false;
        values.insert(item.first, item.second);
    }
    const auto suppliedState = values.value("state");
    if (suppliedState.size() != state.size()) return false;
    ushort difference = 0;
    for (int i = 0; i < state.size(); ++i) difference |= state[i].unicode() ^ suppliedState[i].unicode();
    if (difference || values.contains("error")) return false;
    client = values.value("client_id", registeredClient);
    if (client.isEmpty() || client == "dynamic_agent_client" || client.size() > 256 ||
        (!registeredClient.isEmpty() && client != registeredClient)) return false;
    code = values.value("code");
    return !code.isEmpty() && code.size() <= 8192;
}
bool SiwcProtocol::verifyIdentity(const QString &jwt, const QJsonObject &jwks, const QString &client,
                                  const QString &nonce, qint64 now, QJsonObject &identity) {
    identity = {};
    if (jwt.size() > 32768 || client.isEmpty() || nonce.isEmpty()) return false;
    const auto parts = jwt.toLatin1().split('.');
    if (parts.size() != 3) return false;
    const auto header = QJsonDocument::fromJson(decode(parts[0])).object();
    if (header.value("alg") != "RS256" || !header.value("kid").isString() || header.value("kid").toString().isEmpty() ||
        header.contains("crit") || header.contains("jku") || header.contains("jwk")) return false;
    QJsonObject selected;
    for (const auto &entry : jwks.value("keys").toArray()) {
        const auto key = entry.toObject();
        if (key.value("kid") != header.value("kid")) continue;
        if (!selected.isEmpty()) return false;
        selected = key;
    }
    if (selected.value("kty") != "RSA" || (selected.contains("alg") && selected.value("alg") != "RS256") ||
        (selected.contains("use") && selected.value("use") != "sig") ||
        !signature(parts[0] + '.' + parts[1], decode(parts[2]), selected)) return false;
    const auto claims = QJsonDocument::fromJson(decode(parts[1])).object();
    const auto aud = claims.value("aud");
    const bool audience = aud.isString() ? aud.toString() == client : aud.isArray() && aud.toArray().contains(client);
    if (!audience || (aud.isArray() && aud.toArray().size() > 1 && claims.value("azp").toString() != client) ||
        claims.value("iss") != "https://auth.openai.com" || claims.value("nonce").toString() != nonce ||
        claims.value("sub").toString().isEmpty() || !claims.value("exp").isDouble() || !claims.value("iat").isDouble() ||
        claims.value("exp").toDouble() <= now - 5 || claims.value("iat").toDouble() > now + 5 ||
        (claims.contains("nbf") && (!claims.value("nbf").isDouble() || claims.value("nbf").toDouble() > now + 5))) return false;
    identity = claims;
    return true;
}
