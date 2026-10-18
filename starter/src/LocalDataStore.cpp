#include "LocalDataStore.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <limits>

namespace {
constexpr int MaxRecords = 1000;
constexpr int MaxBytes = 4 * 1024 * 1024;

bool validId(const QString &id) {
    static const QRegularExpression pattern("\\A[a-zA-Z][a-zA-Z0-9_-]{1,63}\\z");
    return pattern.match(id).hasMatch();
}

bool validPayload(const QString &kind, const QJsonObject &payload) {
    if (payload.size() != 1) return false;
    if (kind == "input") return payload.value("value").isString() && payload.value("value").toString().toUcs4().size() <= 500;
    if (kind != "list" || !payload.value("items").isArray()) return false;
    const auto items = payload.value("items").toArray();
    if (items.size() > 50) return false;
    for (const auto &item : items) if (!item.isString() || item.toString().toUcs4().size() > 500) return false;
    return true;
}

QByteArray inputPayload(const QString &text) {
    return QJsonDocument(QJsonObject{{"value", text}}).toJson(QJsonDocument::Compact);
}
QByteArray listPayload(const QVariantList &items) {
    return QJsonDocument(QJsonObject{{"items", QJsonArray::fromVariantList(items)}}).toJson(QJsonDocument::Compact);
}
}

LocalDataStore::LocalDataStore() : m_connection("mutyva-data-" + QUuid::createUuid().toString(QUuid::WithoutBraces)) {}

LocalDataStore::~LocalDataStore() {
    if (m_db.isValid()) m_db.close();
    m_db = QSqlDatabase();
    if (QSqlDatabase::contains(m_connection)) QSqlDatabase::removeDatabase(m_connection);
}

bool LocalDataStore::fail(QString &error, const QString &message) {
    error = "Локальные данные: " + message;
    return false;
}

bool LocalDataStore::open(const QString &path, const QString &appId, LocalDataSnapshot &snapshot, QString &error) {
    error.clear();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return fail(error, "не удалось создать папку хранилища");
    m_appId = appId;
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connection);
    m_db.setDatabaseName(path);
    m_db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=250");
    if (!m_db.open()) return fail(error, "не удалось открыть SQLite: " + m_db.lastError().databaseText());
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA synchronous=FULL")) return fail(error, "не удалось настроить надёжную запись");
    if (!query.exec("PRAGMA quick_check(1)") || !query.next() || query.value(0).toString() != "ok")
        return fail(error, "файл SQLite повреждён; запись заблокирована");
    if (!query.exec("PRAGMA user_version") || !query.next()) return fail(error, "не удалось прочитать версию хранилища");
    const int version = query.value(0).toInt();
    query.finish();
    if (version != 0 && version != 1) return fail(error, "версия хранилища не поддерживается");
    if (version == 0) {
        if (!query.exec("SELECT count(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'") ||
            !query.next() || query.value(0).toInt() != 0) return fail(error, "неизвестная структура базы");
        query.finish();
        if (!m_db.transaction()) return fail(error, "хранилище занято");
        const bool created = query.exec("CREATE TABLE app_state(app_id TEXT PRIMARY KEY, revision INTEGER NOT NULL)") &&
            query.exec("CREATE TABLE widget_data(app_id TEXT NOT NULL, widget_id TEXT NOT NULL, kind TEXT NOT NULL, payload TEXT NOT NULL, PRIMARY KEY(app_id, widget_id, kind))") &&
            query.exec("PRAGMA user_version=1");
        if (!created || !m_db.commit()) { m_db.rollback(); return fail(error, "не удалось создать структуру SQLite"); }
    }
    // Validate existing rows before inserting anything, so corrupt logical data is not repaired silently.
    if (!load(snapshot, error)) return false;
    m_current = snapshot;
    return true;
}

bool LocalDataStore::load(LocalDataSnapshot &snapshot, QString &error) {
    LocalDataSnapshot loaded;
    if (!m_db.transaction()) return fail(error, "не удалось начать чтение");
    auto reject = [&](const QString &message) { m_db.rollback(); return fail(error, message); };
    QSqlQuery query(m_db);
    query.prepare("SELECT revision FROM app_state WHERE app_id=?"); query.addBindValue(m_appId);
    if (!query.exec()) return reject("ошибка таблицы версий");
    const bool hasState = query.next();
    if (hasState) {
        bool ok = false;
        loaded.revision = query.value(0).toString().toLongLong(&ok);
        if (!ok || loaded.revision < 0 || loaded.revision == std::numeric_limits<qint64>::max())
            return reject("некорректная версия данных");
    }
    query.finish();
    query.prepare("SELECT widget_id, kind, payload, length(CAST(payload AS BLOB)) FROM widget_data WHERE app_id=?");
    query.addBindValue(m_appId);
    if (!query.exec()) return reject("ошибка таблицы данных");
    int count = 0;
    qint64 total = 0;
    while (query.next()) {
        const auto id = query.value(0).toString();
        const auto kind = query.value(1).toString();
        total += query.value(3).toLongLong();
        if (++count > MaxRecords || total > MaxBytes || !validId(id) || !hasState)
            return reject("нарушены лимиты или структура записей; запись заблокирована");
        QJsonParseError parseError;
        const auto data = QJsonDocument::fromJson(query.value(2).toString().toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !data.isObject() || !validPayload(kind, data.object()))
            return reject("повреждена запись " + id + "; запись заблокирована");
        if (kind == "input") loaded.inputs.insert(id, data.object().value("value").toString());
        else loaded.lists.insert(id, data.object().value("items").toArray().toVariantList());
    }
    query.finish();
    if (!m_db.commit()) return reject("не удалось завершить чтение");
    snapshot = loaded;
    return true;
}

bool LocalDataStore::save(LocalDataSnapshot &candidate, QString &error) {
    if (!m_db.isOpen()) return fail(error, "хранилище не открыто");
    for (auto it = m_current.inputs.begin(); it != m_current.inputs.end(); ++it)
        if (!candidate.inputs.contains(it.key())) return fail(error, "удаление сохранённых записей этой операцией запрещено");
    for (auto it = m_current.lists.begin(); it != m_current.lists.end(); ++it)
        if (!candidate.lists.contains(it.key())) return fail(error, "удаление сохранённых записей этой операцией запрещено");
    QMap<QString, QPair<QString, QByteArray>> records;
    qint64 total = 0;
    for (auto it = candidate.inputs.begin(); it != candidate.inputs.end(); ++it) {
        const auto payload = inputPayload(it.value());
        if (!validId(it.key()) || !validPayload("input", QJsonDocument::fromJson(payload).object()))
            return fail(error, "недопустимое поле ввода");
        records.insert("input:" + it.key(), {it.key(), payload}); total += payload.size();
    }
    for (auto it = candidate.lists.begin(); it != candidate.lists.end(); ++it) {
        const auto payload = listPayload(it.value());
        if (!validId(it.key()) || !validPayload("list", QJsonDocument::fromJson(payload).object()))
            return fail(error, "недопустимый список");
        records.insert("list:" + it.key(), {it.key(), payload}); total += payload.size();
    }
    if (records.size() > MaxRecords || total > MaxBytes) return fail(error, "превышен лимит хранилища");
    QSqlQuery query(m_db);
    if (!query.exec("BEGIN IMMEDIATE")) return fail(error, "хранилище занято или недоступно для записи");
    auto reject = [&](const QString &message) { m_db.rollback(); return fail(error, message); };
    query.prepare("SELECT revision FROM app_state WHERE app_id=?"); query.addBindValue(m_appId);
    if (!query.exec()) return reject("ошибка чтения версии");
    const qint64 revision = query.next() ? query.value(0).toLongLong() : 0;
    query.finish();
    if (revision != candidate.revision || revision != m_current.revision)
        return reject("данные изменены другим окном. Нажмите «Перечитать данные»");
    if (revision == std::numeric_limits<qint64>::max()) return reject("исчерпаны номера версий");
    query.prepare("INSERT INTO widget_data(app_id,widget_id,kind,payload) VALUES(?,?,?,?) ON CONFLICT(app_id,widget_id,kind) DO UPDATE SET payload=excluded.payload");
    for (auto it = records.begin(); it != records.end(); ++it) {
        const QString kind = it.key().startsWith("input:") ? "input" : "list";
        const auto id = it.value().first;
        if (kind == "input" && m_current.inputs.contains(id) && m_current.inputs.value(id) == candidate.inputs.value(id)) continue;
        if (kind == "list" && m_current.lists.contains(id) && m_current.lists.value(id) == candidate.lists.value(id)) continue;
        query.bindValue(0, m_appId); query.bindValue(1, id); query.bindValue(2, kind);
        query.bindValue(3, QString::fromUtf8(it.value().second));
        if (!query.exec()) return reject("не удалось записать данные");
    }
    query.prepare("INSERT INTO app_state(app_id,revision) VALUES(?,?) ON CONFLICT(app_id) DO UPDATE SET revision=excluded.revision");
    query.addBindValue(m_appId); query.addBindValue(revision + 1);
    if (!query.exec() || !m_db.commit()) return reject("не удалось завершить сохранение");
    candidate.revision = revision + 1;
    m_current = candidate;
    error.clear();
    return true;
}
