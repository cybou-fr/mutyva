#pragma once
#include <QObject>
#include <QVariantList>
#include <QJsonObject>
#include <QString>

class AppDocument : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList widgets READ widgets NOTIFY documentChanged)
    Q_PROPERTY(int generation READ generation NOTIFY documentChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY documentChanged)
    Q_PROPERTY(QVariantList catalog READ catalog CONSTANT)
    Q_PROPERTY(QString documentJson READ documentJson NOTIFY documentChanged)
    Q_PROPERTY(bool hasPreview READ hasPreview NOTIFY documentChanged)
    Q_PROPERTY(QVariantList previewWidgets READ previewWidgets NOTIFY documentChanged)
    Q_PROPERTY(QString previewSummary READ previewSummary NOTIFY documentChanged)
public:
    explicit AppDocument(QObject *parent = nullptr, const QString &storageFolder = {});
    QVariantList widgets() const;
    QVariantList catalog() const;
    QString documentJson() const;
    bool hasPreview() const { return !m_candidate.isEmpty(); }
    QVariantList previewWidgets() const;
    QString previewSummary() const { return m_summary; }
    int generation() const { return m_generation; }
    QString lastError() const { return m_error; }
    Q_INVOKABLE bool addHumidityCard();
    Q_INVOKABLE bool rollback();
    Q_INVOKABLE bool resetDemo();
    Q_INVOKABLE bool previewMutation(const QString &json);
    Q_INVOKABLE bool previewDocument(const QString &json);
    Q_INVOKABLE bool addWidget(const QString &kind);
    Q_INVOKABLE bool removeWidget(const QString &id);
    Q_INVOKABLE bool renameWidget(const QString &id, const QString &title);
    Q_INVOKABLE bool moveWidget(const QString &id, const QString &after);
    Q_INVOKABLE bool applyPreview();
    Q_INVOKABLE void cancelPreview();
signals:
    void documentChanged();
private:
    bool loadDocument(const QString &path, QJsonObject &out);
    bool commit(const QJsonObject &newDocument);
    bool propose(const QJsonObject &operation, const QString &summary);
    bool stage(const QJsonObject &candidate, const QString &summary);
    bool reject(const QString &error);
    QString generationPath(int generation) const;
    QString m_folder;
    QString m_error;
    int m_generation = 0;
    QJsonObject m_document;
    QJsonObject m_candidate;
    QString m_summary;
    bool m_storageReady = true;
};
