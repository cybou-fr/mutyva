#include "DeclarativeRuntime.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <QMap>

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
        const auto hash = ref.indexOf('#');
        const auto file = hash < 0 ? ref : ref.left(hash);
        const auto target = file.isEmpty() ? root : schema(file.left(file.size() - QString(".schema.json").size()));
        QJsonValue resolved(target);
        if (hash >= 0 && hash + 1 < ref.size())
            for (const auto &part : ref.mid(hash + 2).split('/'))
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

int indexOf(const QJsonArray &widgets, const QString &id) {
    for (qsizetype i = 0; i < widgets.size(); ++i)
        if (widgets[i].toObject().value("id").toString() == id) return static_cast<int>(i);
    return -1;
}
}

QVariantList DeclarativeRuntime::catalog() {
    static const QVariantList result = [] {
        QFile file(":/runtime/component-catalog.json");
        if (!file.open(QIODevice::ReadOnly)) return QVariantList{};
        auto components = QJsonDocument::fromJson(file.readAll()).object().value("components").toArray();
        const auto properties = schema("app").value("$defs").toObject().value("widget").toObject()
                                    .value("properties").toObject();
        for (qsizetype i = 0; i < components.size(); ++i) {
            auto entry = components[i].toObject();
            entry.insert("version", 2);
            QJsonObject writable;
            for (const auto &name : entry.value("propertyNames").toArray())
                writable.insert(name.toString(), properties.value(name.toString()));
            entry.remove("propertyNames");
            entry.insert("writableProperties", writable);
            components[i] = entry;
        }
        return components.toVariantList();
    }();
    return result;
}

QJsonObject DeclarativeRuntime::bundledSchema(const QString &name) {
    if (name != "app" && name != "mutation" && name != "designer-response" &&
        name != "app-v1" && name != "mutation-v1") return {};
    return schema(name);
}

bool DeclarativeRuntime::validateDesignerResponse(const QJsonObject &response, QString &error) {
    error.clear();
    const auto rules = schema("designer-response");
    if (rules.isEmpty()) return fail(error, "$", "Схема ответа дизайнера недоступна");
    return check(response, rules, rules, "$", error);
}

bool DeclarativeRuntime::validateDocument(const QJsonObject &document, QString &error) {
    error.clear();
    const auto rules = schema(document.value("schemaVersion") == QJsonValue(1) ? "app-v1" : "app");
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
    QMap<QString, QJsonObject> byId;
    QSet<QString> containers;
    QString defaultCity;
    for (const auto &entry : document.value("widgets").toArray()) {
        const auto widget = entry.toObject();
        const auto id = widget.value("id").toString();
        if (widget.value("kind") == "city-search" && widget.contains("defaultCity")) {
            const auto city = widget.value("defaultCity").toString().trimmed();
            if (city.size() < 2) return fail(error, "$.widgets." + id, "Нужен город не короче двух символов");
            if (!defaultCity.isEmpty() && defaultCity.compare(city, Qt::CaseInsensitive) != 0)
                return fail(error, "$.widgets." + id, "Все поиски города должны использовать один defaultCity");
            defaultCity = city;
        }
        if (ids.contains(id)) return fail(error, "$.widgets", "Повтор ID " + id);
        ids.insert(id);
        byId.insert(id, widget);
        bool registered = false;
        for (const auto &entry : catalog()) {
            const auto c = entry.toMap();
            if (widget.value("kind").toString() != c.value("kind").toString()) continue;
            registered = true;
            if (c.value("container").toBool()) containers.insert(id);
            if (!c.value("source").toString().isEmpty() && !sources.contains(c.value("source").toString()))
                return fail(error, "$.widgets." + id, "Нет источника " + c.value("source").toString());
            const auto allowed = c.value("writableProperties").toMap();
            for (auto it = widget.begin(); it != widget.end(); ++it)
                if (it.key() != "id" && it.key() != "kind" && it.key() != "parent" && !allowed.contains(it.key()))
                    return fail(error, "$.widgets." + id, "Свойство недоступно этому виду: " + it.key());
        }
        if (!registered) return fail(error, "$.widgets." + id, "Компонент не зарегистрирован");
    }
    for (auto it = byId.begin(); it != byId.end(); ++it) {
        QString parent = it.value().value("parent").toString();
        QSet<QString> visited{it.key()};
        while (!parent.isEmpty()) {
            if (!containers.contains(parent)) return fail(error, it.key(), "parent должен ссылаться на контейнер: " + parent);
            if (visited.contains(parent)) return fail(error, it.key(), "Цикл вложенности");
            visited.insert(parent);
            if (visited.size() > 8) return fail(error, it.key(), "Превышена глубина 8 уровней");
            parent = byId.value(parent).value("parent").toString();
        }
        const auto action = it.value().value("action").toObject();
        if (action.isEmpty()) continue;
        const auto target = action.value("target").toString();
        if (byId.value(target).value("kind").toString() != "list")
            return fail(error, it.key(), "Действию нужен существующий список target");
        if (action.value("kind").toString() == "append-item" &&
            byId.value(action.value("input").toString()).value("kind").toString() != "input")
            return fail(error, it.key(), "Действию нужно существующее поле input");
    }
    return true;
}

bool DeclarativeRuntime::mutate(const QJsonObject &document, const QJsonObject &proposal,
                               QJsonObject &candidate, QString &error) {
    if (!validateDocument(document, error)) return false;
    const auto rules = schema(proposal.value("schemaVersion") == QJsonValue(1) ? "mutation-v1" : "mutation");
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
                if (after >= 0 && widgets[after].toObject().value("parent").toString() != widget.value("parent").toString())
                    return fail(error, "addWidget", "after должен быть соседом в том же контейнере");
            }
            widgets.insert(after + 1, widget);
        } else {
            if (index < 0) return fail(error, name, "Не найден виджет " + id);
            if (name == "removeWidget") {
                QSet<QString> removed{id};
                bool grew = true;
                while (grew) {
                    grew = false;
                    for (const auto &entry : widgets) {
                        const auto w = entry.toObject();
                        const auto child = w.value("id").toString();
                        if (removed.contains(w.value("parent").toString()) && !removed.contains(child)) {
                            removed.insert(child); grew = true;
                        }
                    }
                }
                for (qsizetype i = widgets.size(); i > 0; --i)
                    if (removed.contains(widgets[i - 1].toObject().value("id").toString())) widgets.removeAt(i - 1);
            }
            else if (name == "updateProperty") {
                auto widget = widgets[index].toObject();
                widget.insert(op.value("path").toString(), op.value("value"));
                widgets[index] = widget;
            } else if (name == "moveWidget") {
                const auto anchor = op.value("after").toString();
                if (anchor == id) return fail(error, name, "Нельзя переместить виджет после себя");
                auto widget = widgets.takeAt(index).toObject();
                if (op.contains("parent")) {
                    if (op.value("parent").toString().isEmpty()) widget.remove("parent");
                    else widget.insert("parent", op.value("parent"));
                }
                const int after = indexOf(widgets, anchor);
                if (!anchor.isEmpty() && after < 0) return fail(error, name, "Не найден after: " + anchor);
                if (after >= 0 && widgets[after].toObject().value("parent").toString() != widget.value("parent").toString())
                    return fail(error, name, "after должен быть соседом в том же контейнере");
                widgets.insert(after + 1, widget);
            }
        }
    }
    QJsonObject next = document;
    next.insert("schemaVersion", 2);
    next.insert("widgets", widgets);
    if (!validateDocument(next, error)) return false;
    candidate = next; // Only publish a complete, validated result.
    return true;
}
