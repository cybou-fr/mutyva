#include "LiveAiCheck.h"
#include "AppDocument.h"
#include "AiChat.h"
#include "ChatSession.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QSaveFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace {
const QStringList Names{"conversation_reply", "mutation_preview_apply_restart", "default_city_apply_restart"};
const QStringList Requests{
    "Не меняй интерфейс. Задай один короткий уточняющий вопрос: какой виджет погоды пользователь хочет добавить. Верни clarification.",
    "Добавь одну отдельную карточку влажности humidity-card сразу после forecast. Используй существующий источник weather. Сохрани все существующие виджеты, их свойства и порядок. Не добавляй другие изменения.",
    "Показывай по умолчанию погоду в Poitiers. Сохрани остальной интерфейс без изменений."
};
}
LiveAiCheck::LiveAiCheck(ChatSession *session, const QString &reportPath, QObject *parent, QNetworkAccessManager *network)
    : QObject(parent), m_session(session), m_network(network), m_reportPath(reportPath) {
    m_deadline.setSingleShot(true);
    connect(&m_deadline, &QTimer::timeout, this, [this] { finish("failed", "test_deadline", 1); });
    connect(session, &ChatSession::changed, this, [this] {
        if (!m_started || m_finished || m_case >= 0) return;
        if (m_session->busy()) m_authBusy = true;
        if (m_session->connected()) QTimer::singleShot(0, this, &LiveAiCheck::nextCase);
        else if (m_authBusy && !m_session->busy()) finish("failed", "sign_in_failed_or_cancelled", 1);
    });
}
LiveAiCheck::~LiveAiCheck() = default;
void LiveAiCheck::start() {
    if (m_started) return;
    m_started = true;
    if (!publish("waiting_for_sign_in", "human_sign_in_required", -1)) { finish("failed", "report_unavailable", 3); return; }
    m_deadline.start(10 * 60 * 1000);
    if (!m_store.isValid()) { finish("failed", "temporary_store_unavailable", 1); return; }
    if (m_session->connected()) QTimer::singleShot(0, this, &LiveAiCheck::nextCase);
    emit changed();
}
void LiveAiCheck::cancel() {
    if (m_finished) return;
    finish("cancelled", "user_cancelled", 2);
    m_session->cancel();
}
void LiveAiCheck::nextCase() {
    if (m_finished || m_inFlight || !m_session->connected()) return;
    ++m_case;
    if (m_case >= Requests.size()) { finish("passed", "all_checks_passed", 0); return; }
    m_chat.reset();
    m_document = std::make_unique<AppDocument>(nullptr, m_store.filePath(QString::number(m_case)));
    m_chat = std::make_unique<AiChat>(m_document.get(), m_session, nullptr, m_network);
    connect(m_chat.get(), &AiChat::changed, this, [this] { QTimer::singleShot(0, this, &LiveAiCheck::checkReply); });
    m_status = QString("Проверка %1/%2: %3").arg(m_case + 1).arg(Requests.size()).arg(m_case == 0 ? "получение ответа AI" : "изменение и восстановление тестового приложения");
    m_deadline.start(4 * 60 * 1000);
    m_elapsed.start(); m_inFlight = true;
    if (!publish("running", Names[m_case], -1)) { finish("failed", "report_unavailable", 3); return; }
    emit changed();
    if (!m_chat->send(Requests[m_case])) checkReply();
}
void LiveAiCheck::record(const QString &name, bool passed, const QString &reason, const QString &reply) {
    m_results.append(QVariantMap{{"name", name}, {"passed", passed}, {"reason", reason},
        {"elapsedMs", m_elapsed.isValid() ? m_elapsed.elapsed() : 0}, {"reply", reply.left(600)},
        {"diagnostics", m_chat ? m_chat->diagnostics() : QString()}});
    emit changed();
}
void LiveAiCheck::checkReply() {
    if (m_finished || !m_inFlight || !m_chat || m_chat->busy()) return;
    m_inFlight = false;
    const auto messages = m_chat->messages();
    const auto last = messages.isEmpty() ? QVariantMap() : messages.last().toMap();
    if (last.value("role") != "assistant" || last.value("text").toString().isEmpty()) {
        record(Names[m_case], false, "no_valid_assistant_response");
        finish("failed", "no_valid_assistant_response", 1); return;
    }
    const auto reply = last.value("text").toString();
    if (m_case == 0) {
        const bool passed = !m_document->hasPreview() && m_document->generation() == 1;
        record(Names[m_case], passed, passed ? "validated_reply_without_mutation" : "unexpected_mutation", reply);
        if (!passed) { finish("failed", "unexpected_mutation", 1); return; }
    } else {
        if (!m_document->hasPreview()) {
            record(Names[m_case], false, "mutation_not_proposed", reply); finish("failed", "mutation_not_proposed", 1); return;
        }
        const auto original = m_document->widgets();
        const auto candidate = m_document->previewWidgets();
        bool preserved = false;
        if (m_case == 1) {
            preserved = candidate.size() == original.size() + 1;
            for (const auto &widget : original) preserved = preserved && candidate.contains(widget);
            int forecast = -1, humidity = -1;
            for (int i = 0; i < candidate.size(); ++i) {
                const auto widget = candidate[i].toMap();
                if (widget.value("id") == "forecast") forecast = i;
                if (widget.value("kind") == "humidity-card") humidity = i;
            }
            preserved = preserved && forecast >= 0 && humidity == forecast + 1;
        } else {
            preserved = candidate.size() == original.size() && m_document->defaultCity() == "Paris";
            for (int i = 0; preserved && i < candidate.size(); ++i) {
                auto widget = candidate[i].toMap();
                if (widget.value("id") == "citySearch") {
                    preserved = widget.take("defaultCity").toString().trimmed().compare("Poitiers", Qt::CaseInsensitive) == 0;
                }
                preserved = preserved && widget == original[i].toMap();
            }
        }
        if (!preserved) {
            record(Names[m_case], false, "unexpected_candidate", reply); finish("failed", "unexpected_candidate", 1); return;
        }
        if (!m_document->applyPreview()) {
            record(Names[m_case], false, "temporary_commit_failed", reply); finish("failed", "temporary_commit_failed", 1); return;
        }
        AppDocument restored(nullptr, m_store.filePath(QString::number(m_case)));
        const bool persisted = restored.generation() == 2 && restored.documentJson() == m_document->documentJson() &&
            (m_case != 2 || restored.defaultCity().compare("Poitiers", Qt::CaseInsensitive) == 0);
        record(Names[m_case], persisted, persisted ? (m_case == 2 ? "default_city_restart_verified" : "preview_apply_restart_verified") : "restart_mismatch", reply);
        if (!persisted) { finish("failed", "restart_mismatch", 1); return; }
    }
    QTimer::singleShot(0, this, &LiveAiCheck::nextCase);
}
void LiveAiCheck::finish(const QString &status, const QString &reason, int exitCode) {
    if (m_finished) return;
    m_finished = true;
    if (m_case >= 0 && m_case < Names.size() && m_results.size() <= m_case)
        record(Names[m_case], false, reason);
    m_inFlight = false; m_deadline.stop();
    if (m_chat && m_chat->busy()) m_chat->cancel();
    m_status = status == "passed" ? "Проверка пройдена. Ответ AI, предпросмотр, сохранение и восстановление работают." :
        status == "cancelled" ? "Проверка отменена." : "Проверка завершилась ошибкой. Отчёт сохранён для диагностики.";
    const bool saved = publish(status, reason, exitCode);
    if (!saved) { m_status = "Не удалось сохранить отчёт проверки."; exitCode = 3; }
    qInfo().noquote() << "LIVE_AI_TEST" << status << reason << "exit=" << exitCode;
    emit changed(); emit completed(exitCode);
}

bool LiveAiCheck::publish(const QString &status, const QString &reason, int exitCode) {
    QJsonObject report{{"status", status}, {"reason", reason}, {"exitCode", exitCode},
        {"updatedAt", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}, {"isolatedApplication", true},
        {"cases", QJsonArray::fromVariantList(m_results)}};
    if (m_finished) report["finishedAt"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSaveFile file(m_reportPath);
    const auto data = QJsonDocument(report).toJson();
    return QDir().mkpath(QFileInfo(m_reportPath).absolutePath()) && file.open(QIODevice::WriteOnly) &&
        file.write(data) == data.size() && file.commit();
}
