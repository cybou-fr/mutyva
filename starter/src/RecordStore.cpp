#include "RecordStore.h"
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <limits>

namespace {
bool validName(const QString &name) {
    static const QRegularExpression pattern("^[a-zA-Z][a-zA-Z0-9_-]{1,63}$");
    return pattern.match(name).hasMatch();
}
bool fail(QString &error, const QString &message) {
    error = "Записи: " + message;
    return false;
}
}

RecordStore::RecordStore()
    : m_connection("mutyva-records-" + QUuid::createUuid().toString(QUuid::WithoutBraces)) {}

RecordStore::~RecordStore() {
    if (m_db.isValid()) m_db.close();
    m_db = QSqlDatabase();
    if (QSqlDatabase::contains(m_connection)) QSqlDatabase::removeDatabase(m_connection);
}

bool RecordStore::open(const QString &path, const QString &appId, QString &error) {
    if (!QDir().mkpath(QFileInfo(path).absolutePath()) || appId.isEmpty())
        return fail(error, "некорректный путь или ID приложения");
    m_appId = appId;
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connection);
    m_db.setDatabaseName(path);
    m_db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=250");
    if (!m_db.open()) return fail(error, "не удалось открыть SQLite");
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA synchronous=FULL") ||
        !query.exec("PRAGMA quick_check(1)") || !query.next() ||
        query.value(0).toString() != "ok")
        return fail(error, "SQLite повреждён либо недоступен");
    query.finish();
    // Same database as LocalDataStore; additive tables, no modification of legacy widget_data.
    if (!query.exec("CREATE TABLE IF NOT EXISTS record_state(app_id TEXT PRIMARY KEY, revision INTEGER NOT NULL)"))
        return fail(error, "не удалось создать версии записей");
    if (!query.exec("CREATE TABLE IF NOT EXISTS record_data(app_id TEXT NOT NULL, model_id TEXT NOT NULL, record_id TEXT NOT NULL, payload TEXT NOT NULL, PRIMARY KEY(app_id, model_id, record_id))"))
        return fail(error, "не удалось создать таблицу записей");
    query.prepare("SELECT revision FROM record_state WHERE app_id=?");
    query.addBindValue(m_appId);
    if (!query.exec()) return fail(error, "не удалось прочитать версию");
    if (query.next()) {
        bool ok = false;
        m_revision = query.value(0).toString().toLongLong(&ok);
        if (!ok || m_revision < 0 || m_revision == std::numeric_limits<qint64>::max())
            return fail(error, "повреждена версия записей");
    } else m_revision = 0;
    query.finish();
    query.prepare("SELECT count(*), coalesce(sum(length(CAST(payload AS BLOB))),0) FROM record_data WHERE app_id=?");
    query.addBindValue(m_appId);
    if (!query.exec() || !query.next() || query.value(0).toInt() > 1000 ||
        query.value(1).toLongLong() > 4 * 1024 * 1024)
        return fail(error, "превышен лимит записей");
    error.clear();
    return true;
}

bool RecordStore::list(const QString &modelId, QJsonArray &result, QString &error) {
    if (!m_db.isOpen() || !validName(modelId)) return fail(error, "неизвестная модель");
    QSqlQuery query(m_db);
    query.prepare("SELECT record_id,payload FROM record_data WHERE app_id=? AND model_id=? ORDER BY rowid");
    query.addBindValue(m_appId);
    query.addBindValue(modelId);
    if (!query.exec()) return fail(error, "не удалось прочитать записи");
    QJsonArray candidate;
    while (query.next()) {
        const auto id = query.value(0).toString();
        QJsonParseError e;
        const auto data = QJsonDocument::fromJson(query.value(1).toByteArray(), &e);
        if (!validName(id) || e.error != QJsonParseError::NoError || !data.isObject())
            return fail(error, "запись повреждена; восстановление не выполняется автоматически");
        auto item = data.object();
        item.insert("_id", id);
        candidate.append(item);
        if (candidate.size() > 1000) return fail(error, "слишком много записей");
    }
    result = candidate;
    error.clear();
    return true;
}

bool RecordStore::create(const QString &modelId, const QJsonObject &values, QString &recordId, QString &error) {
    const QString id = "r" + QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
    if (!change("insert", modelId, id, values, error)) return false;
    recordId = id;
    return true;
}
bool RecordStore::update(const QString &modelId, const QString &recordId,
                         const QJsonObject &values, QString &error) {
    return change("update", modelId, recordId, values, error);
}
bool RecordStore::remove(const QString &modelId, const QString &recordId, QString &error) {
    return change("delete", modelId, recordId, {}, error);
}

bool RecordStore::change(const QString &op, const QString &modelId, const QString &recordId,
                         const QJsonObject &values, QString &error) {
    if (!m_db.isOpen() || !validName(modelId) || !validName(recordId) ||
        QJsonDocument(values).toJson(QJsonDocument::Compact).size() > 16 * 1024)
        return fail(error, "недопустимая операция или размер записи");
    QSqlQuery query(m_db);
    if (!query.exec("BEGIN IMMEDIATE")) return fail(error, "хранилище занято");
    auto reject = [&](const QString &why) { m_db.rollback(); return fail(error, why); };
    query.prepare("SELECT revision FROM record_state WHERE app_id=?");
    query.addBindValue(m_appId);
    if (!query.exec()) return reject("ошибка чтения версии");
    const qint64 current = query.next() ? query.value(0).toLongLong() : 0;
    query.finish();
    if (current != m_revision || current == std::numeric_limits<qint64>::max())
        return reject("данные изменены другим окном. Нажмите «Перечитать данные»");
    if (op == "insert") {
        query.prepare("INSERT INTO record_data(app_id,model_id,record_id,payload) VALUES(?,?,?,?)");
        query.addBindValue(m_appId); query.addBindValue(modelId); query.addBindValue(recordId);
        query.addBindValue(QString::fromUtf8(QJsonDocument(values).toJson(QJsonDocument::Compact)));
    } else if (op == "update") {
        query.prepare("UPDATE record_data SET payload=? WHERE app_id=? AND model_id=? AND record_id=?");
        query.addBindValue(QString::fromUtf8(QJsonDocument(values).toJson(QJsonDocument::Compact)));
        query.addBindValue(m_appId); query.addBindValue(modelId); query.addBindValue(recordId);
    } else if (op == "delete") {
        query.prepare("DELETE FROM record_data WHERE app_id=? AND model_id=? AND record_id=?");
        query.addBindValue(m_appId); query.addBindValue(modelId); query.addBindValue(recordId);
    } else return reject("неизвестное действие");
    if (!query.exec()) return reject("операция SQLite не выполнена");
    if (op != "insert" && query.numRowsAffected() != 1)
        return reject("запись больше не существует");
    query.finish();
    query.prepare("SELECT count(*),coalesce(sum(length(CAST(payload AS BLOB))),0) FROM record_data WHERE app_id=?");
    query.addBindValue(m_appId);
    if (!query.exec() || !query.next() || query.value(0).toInt() > 1000 ||
        query.value(1).toLongLong() > 4 * 1024 * 1024)
        return reject("превышен лимит 1000 записей / 4 МиБ");
    query.finish();
    query.prepare("INSERT INTO record_state(app_id,revision) VALUES(?,?) ON CONFLICT(app_id) DO UPDATE SET revision=excluded.revision");
    query.addBindValue(m_appId); query.addBindValue(current + 1);
    if (!query.exec()) return reject("не удалось записать версию");
    query.finish();
    if (!m_db.commit()) return reject("транзакция не завершена");
    m_revision = current + 1;
    error.clear();
    return true;
}
