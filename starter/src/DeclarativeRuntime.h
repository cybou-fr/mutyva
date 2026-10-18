#pragma once
#include <QJsonObject>
#include <QVariantList>

// Implements the vocabulary used by the bundled schemas, not a general schema library.
class DeclarativeRuntime {
public:
    static bool validateDocument(const QJsonObject &document, QString &error);
    static bool mutate(const QJsonObject &document, const QJsonObject &proposal,
                       QJsonObject &candidate, QString &error);
    static QVariantList catalog();
    static QJsonObject bundledSchema(const QString &name);
    static bool validateDesignerResponse(const QJsonObject &response, QString &error);
};
