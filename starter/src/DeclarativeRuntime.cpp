#include "DeclarativeRuntime.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <QMap>
#include <QtMath>
#include <QDate>

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
        (type == "array" && value.isArray()) || (type == "string" && value.isString()) ||
        (type == "boolean" && value.isBool()) ||
        (type == "number" && value.isDouble()) ||
        (type == "integer" && value.isDouble() && value.toDouble() == qFloor(value.toDouble()))))
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
    if (value.isDouble() && ((rule.contains("minimum") && value.toDouble() < rule.value("minimum").toDouble()) ||
        (rule.contains("maximum") && value.toDouble() > rule.value("maximum").toDouble())))
        return fail(error, path, "Число за пределами диапазона");
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
    QMap<QString, QJsonObject> models;
    const QRegularExpression isoDate("^\\d{4}-\\d{2}-\\d{2}$");
    auto validValue = [&](const QString &type, const QJsonValue &value) {
        if (type == "text") return value.isString() && value.toString().size() <= 1000;
        if (type == "boolean") return value.isBool();
        if (type == "integer") return value.isDouble() && value.toDouble() == qFloor(value.toDouble());
        if (type == "date") return value.isString() && isoDate.match(value.toString()).hasMatch() &&
            QDate::fromString(value.toString(), Qt::ISODate).isValid();
        return false;
    };
    for (const auto &item : document.value("models").toArray()) {
        const auto model = item.toObject();
        const auto id = model.value("id").toString();
        if (models.contains(id)) return fail(error, "$.models", "Повтор модели " + id);
        models.insert(id, model);
        QSet<QString> fieldIds;
        for (const auto &entry : model.value("fields").toArray()) {
            const auto field = entry.toObject();
            const auto fieldId = field.value("id").toString();
            if (fieldIds.contains(fieldId)) return fail(error, "$.models." + id, "Повтор поля " + fieldId);
            fieldIds.insert(fieldId);
            if (field.contains("default") && !validValue(field.value("type").toString(), field.value("default")))
                return fail(error, "$.models." + id + "." + fieldId, "Недопустимое значение по умолчанию");
            if (field.value("required").toBool() && !field.contains("default"))
                return fail(error, "$.models." + id + "." + fieldId, "Обязательное поле требует default для безопасных миграций");
        }
    }
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
        const auto kind = it.value().value("kind").toString();
        if (kind == "record-list" || kind == "record-form") {
            const auto modelId = it.value().value("model").toString();
            if (!models.contains(modelId)) return fail(error, it.key(), "Неизвестная модель " + modelId);
            const auto fields = models.value(modelId).value("fields").toArray();
            auto fieldType = [&](const QString &fieldId) {
                for (const auto &entry : fields)
                    if (entry.toObject().value("id").toString() == fieldId)
                        return entry.toObject().value("type").toString();
                return QString();
            };
            for (const auto &key : {"filter", "sort"}) if (it.value().contains(key)) {
                const auto ref = it.value().value(key).toObject();
                const auto type = fieldType(ref.value("field").toString());
                if (type.isEmpty()) return fail(error, it.key(), "Фильтр/сортировка: поле отсутствует");
                if (QString(key) == "filter" && (!validValue(type, ref.value("value")) ||
                    (ref.value("op") != QJsonValue("equals") && type != "date" && type != "integer")))
                    return fail(error, it.key(), "Неверный тип или оператор фильтра");
            }
        }
        if (kind == "select" && it.value().contains("value") &&
            !it.value().value("options").toArray().contains(it.value().value("value")))
            return fail(error, it.key(), "Выбранное значение отсутствует в options");
        if (kind == "checkbox" && it.value().contains("value") &&
            it.value().value("value") != QJsonValue("true") && it.value().value("value") != QJsonValue("false"))
            return fail(error, it.key(), "Checkbox принимает только true/false");
        if (kind == "date-picker" && !it.value().value("value").toString().isEmpty() &&
            !QDate::fromString(it.value().value("value").toString(), Qt::ISODate).isValid())
            return fail(error, it.key(), "Недопустимая ISO-дата");
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
    auto models = document.value("models").toArray();
    for (const auto &entry : proposal.value("operations").toArray()) {
        const auto operation = entry.toObject();
        const auto action = operation.value("op").toString();
        if (action == "addModel") {
            const auto incoming = operation.value("model").toObject();
            bool duplicate = false;
            for (const auto &existing : models)
                if (existing.toObject().value("id") == incoming.value("id")) duplicate = true;
            if (duplicate) return fail(error, "addModel", "Модель уже существует");
            models.append(incoming);
            continue;
        }
        if (action == "addField") {
            const auto modelId = operation.value("modelId").toString();
            const auto field = operation.value("field").toObject();
            bool found = false;
            for (qsizetype i = 0; i < models.size(); ++i) {
                auto model = models[i].toObject();
                if (model.value("id").toString() != modelId) continue;
                found = true;
                auto fields = model.value("fields").toArray();
                for (const auto &existing : fields)
                    if (existing.toObject().value("id") == field.value("id"))
                        return fail(error, "addField", "Поле уже существует");
                fields.append(field);
                model.insert("fields", fields);
                models[i] = model;
                break;
            }
            if (!found) return fail(error, "addField", "Модель не найдена");
            continue;
        }
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
    if (document.contains("models") || !models.isEmpty()) next.insert("models", models);
    if (!validateDocument(next, error)) return false;
    candidate = next; // Only publish a complete, validated result.
    return true;
}
