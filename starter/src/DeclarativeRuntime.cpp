#include "DeclarativeRuntime.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>

namespace {
QJsonObject schema(const QString &name) {
    QFile file(":/specs/" + name + ".schema.json");
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

bool fail(QString &error, const QString &path, const QString &reason) {
    error = path + ": " + reason;
    return false;
}

bool check(const QJsonValue &value, const QJsonObject &rule, const QJsonObject &root,
           const QString &path, QString &error) {
    if (rule.contains("$ref")) {
        const QString ref = rule.value("$ref").toString();
        const auto target = ref.startsWith("app.schema.json") ? schema("app") : root;
        QJsonValue resolved(target);
        for (const auto &part : ref.mid(ref.indexOf('#') + 2).split('/'))
            resolved = resolved.toObject().value(part);
        if (!resolved.isObject()) return fail(error, path, "Не найдена схема " + ref);
        return check(value, resolved.toObject(), target, path, error);
    }
    if (rule.contains("oneOf")) {
        int matches = 0;
        for (const auto &branch : rule.value("oneOf").toArray()) {
            QString ignored;
            if (check(value, branch.toObject(), root, path, ignored)) ++matches;
        }
        if (matches != 1) return fail(error, path, "Операция не соответствует схеме");
    }
    const QString type = rule.value("type").toString();
    if ((!type.isEmpty()) && !((type == "object" && value.isObject()) ||
        (type == "array" && value.isArray()) || (type == "string" && value.isString())))
        return fail(error, path, "Ожидается " + type);
    if (rule.contains("const") && value != rule.value("const"))
        return fail(error, path, "Недопустимое значение");
    if (rule.contains("enum") && !rule.value("enum").toArray().contains(value))
        return fail(error, path, "Значение отсутствует в каталоге");
    if (value.isString()) {
        const auto text = value.toString();
        if ((rule.contains("minLength") && text.toUcs4().size() < rule.value("minLength").toInt()) ||
            (rule.contains("maxLength") && text.toUcs4().size() > rule.value("maxLength").toInt()))
            return fail(error, path, "Недопустимая длина строки");
        if (rule.contains("pattern") && !QRegularExpression(rule.value("pattern").toString()).match(text).hasMatch())
            return fail(error, path, "Недопустимый идентификатор");
    }
    if (value.isObject()) {
        const auto obj = value.toObject();
        for (const auto &key : rule.value("required").toArray())
            if (!obj.contains(key.toString())) return fail(error, path, "Нет поля " + key.toString());
        const auto properties = rule.value("properties").toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (!properties.contains(it.key()) && rule.value("additionalProperties") == QJsonValue(false))
                return fail(error, path, "Запрещённое поле " + it.key());
            if (properties.contains(it.key()) && !check(it.value(), properties.value(it.key()).toObject(), root,
                                                       path + "." + it.key(), error)) return false;
        }
    }
    if (value.isArray()) {
        const auto array = value.toArray();
        if ((rule.contains("minItems") && array.size() < rule.value("minItems").toInt()) ||
            (rule.contains("maxItems") && array.size() > rule.value("maxItems").toInt()))
            return fail(error, path, "Недопустимое число элементов");
        for (qsizetype i = 0; i < array.size(); ++i) {
            if (rule.value("uniqueItems").toBool())
                for (qsizetype j = 0; j < i; ++j)
                    if (array[i] == array[j]) return fail(error, path, "Повторяющийся элемент");
            if (rule.contains("items") && !check(array[i], rule.value("items").toObject(), root,
                                               path + QString("[%1]").arg(i), error)) return false;
        }
    }
    return true;
}

struct Component {
    const char *kind;
    const char *title;
    const char *source;
    const char *binding;
};
const Component components[] = {
    {"city-search", "Поиск города", "geocoding", "city.search"},
    {"weather-card", "Текущая погода", "weather", "weather.current"},
    {"forecast-list", "Прогноз на 7 дней", "weather", "weather.daily"},
    {"air-card", "Качество воздуха", "airQuality", "air.current.european_aqi"},
    {"humidity-card", "Влажность воздуха", "weather", "weather.current.humidity"}
};

int indexOf(const QJsonArray &widgets, const QString &id) {
    for (qsizetype i = 0; i < widgets.size(); ++i)
        if (widgets[i].toObject().value("id").toString() == id) return static_cast<int>(i);
    return -1;
}
}

QVariantList DeclarativeRuntime::catalog() {
    QVariantList result;
    for (const auto &c : components)
        result.append(QVariantMap{{"kind", c.kind}, {"title", QString::fromUtf8(c.title)},
                                  {"source", c.source}, {"binding", c.binding}});
    return result;
}

bool DeclarativeRuntime::validateDocument(const QJsonObject &document, QString &error) {
    error.clear();
    const auto rules = schema("app");
    if (rules.isEmpty()) return fail(error, "$", "Схема приложения недоступна");
    if (!check(document, rules, rules, "$", error)) return false;
    QSet<QString> sources;
    for (const auto &entry : document.value("dataSources").toArray()) {
        const auto id = entry.toObject().value("id").toString();
        if (sources.contains(id)) return fail(error, "$.dataSources", "Повтор ID " + id);
        if (id != "weather" && id != "geocoding" && id != "airQuality")
            return fail(error, "$.dataSources", "Неизвестный источник " + id);
        sources.insert(id);
    }
    const auto permissions = document.value("permissions").toArray();
    for (const auto &permission : permissions)
        if (permission.toString() != "network:*.open-meteo.com")
            return fail(error, "$.permissions", "Разрешение не одобрено");
    if (!sources.isEmpty() && !permissions.contains("network:*.open-meteo.com"))
        return fail(error, "$.permissions", "Для Open-Meteo требуется разрешение сети");
    QSet<QString> ids;
    for (const auto &entry : document.value("widgets").toArray()) {
        const auto widget = entry.toObject();
        const auto id = widget.value("id").toString();
        if (ids.contains(id)) return fail(error, "$.widgets", "Повтор ID " + id);
        ids.insert(id);
        bool registered = false;
        for (const auto &c : components) {
            if (widget.value("kind").toString() != QLatin1String(c.kind)) continue;
            registered = true;
            if (!sources.contains(QLatin1String(c.source)))
                return fail(error, "$.widgets." + id, "Нет источника " + QString::fromLatin1(c.source));
        }
        if (!registered) return fail(error, "$.widgets." + id, "Компонент не зарегистрирован");
    }
    return true;
}

bool DeclarativeRuntime::mutate(const QJsonObject &document, const QJsonObject &proposal,
                               QJsonObject &candidate, QString &error) {
    if (!validateDocument(document, error)) return false;
    const auto rules = schema("mutation");
    if (rules.isEmpty()) return fail(error, "$", "Схема мутации недоступна");
    if (!check(proposal, rules, rules, "$", error)) return false;
    if (proposal.value("appId") != document.value("id"))
        return fail(error, "$.appId", "Мутация предназначена другому приложению");
    auto widgets = document.value("widgets").toArray();
    for (const auto &entry : proposal.value("operations").toArray()) {
        const auto op = entry.toObject();
        const auto name = op.value("op").toString();
        const auto id = op.value("id").toString();
        const int index = indexOf(widgets, id);
        if (name == "addWidget") {
            const auto widget = op.value("widget").toObject();
            if (indexOf(widgets, widget.value("id").toString()) >= 0)
                return fail(error, "addWidget", "ID уже существует");
            int after = static_cast<int>(widgets.size()) - 1;
            if (op.contains("after")) {
                const auto anchor = op.value("after").toString();
                after = indexOf(widgets, anchor);
                if (!anchor.isEmpty() && after < 0) return fail(error, "addWidget", "Не найден after: " + anchor);
            }
            widgets.insert(after + 1, widget);
        } else {
            if (index < 0) return fail(error, name, "Не найден виджет " + id);
            if (name == "removeWidget") widgets.removeAt(index);
            else if (name == "updateProperty") {
                auto widget = widgets[index].toObject();
                widget.insert(op.value("path").toString(), op.value("value"));
                widgets[index] = widget;
            } else if (name == "moveWidget") {
                const auto anchor = op.value("after").toString();
                if (anchor == id) return fail(error, name, "Нельзя переместить виджет после себя");
                const auto widget = widgets.takeAt(index);
                const int after = indexOf(widgets, anchor);
                if (!anchor.isEmpty() && after < 0) return fail(error, name, "Не найден after: " + anchor);
                widgets.insert(after + 1, widget);
            }
        }
    }
    QJsonObject next = document;
    next.insert("widgets", widgets);
    if (!validateDocument(next, error)) return false;
    candidate = next; // Only publish a complete, validated result.
    return true;
}
