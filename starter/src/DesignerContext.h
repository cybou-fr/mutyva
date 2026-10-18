#pragma once
#include <QJsonObject>

// Builds an inspectable local packet. This class performs no network I/O.
class DesignerContext {
public:
    static bool build(const QJsonObject &document, int generation, const QString &request,
                      QJsonObject &packet, QString &error);
    static QString instructions();
};
