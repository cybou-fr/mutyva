#pragma once
#include <QObject>
#include <QVariantList>
#include <QJsonObject>
#include <QString>

class AppDocument : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList widgets READ widgets NOTIFY documentChanged)
    Q_PROPERTY(QString defaultCity READ defaultCity NOTIFY documentChanged)
    Q_PROPERTY(int generation READ generation NOTIFY documentChanged)
    Q_PROPERTY(bool storageReady READ storageReady NOTIFY documentChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY documentChanged)
    Q_PROPERTY(QVariantList catalog READ catalog CONSTANT)
    Q_PROPERTY(QString documentJson READ documentJson NOTIFY documentChanged)
    Q_PROPERTY(bool hasPreview READ hasPreview NOTIFY documentChanged)
    Q_PROPERTY(QVariantList previewWidgets READ previewWidgets NOTIFY documentChanged)
    Q_PROPERTY(QString previewSummary READ previewSummary NOTIFY documentChanged)
    Q_PROPERTY(QString designerContextJson READ designerContextJson NOTIFY designerChanged)
    Q_PROPERTY(QString designerMessage READ designerMessage NOTIFY designerChanged)
public:
    explicit AppDocument(QObject *parent = nullptr, const QString &storageFolder = {});
    QVariantList widgets() const;
    QVariantList catalog() const;
    QString documentJson() const;
    QString defaultCity() const;
    QString userDataPath() const { return m_folder + "/data/user-data.sqlite"; }
    bool hasPreview() const { return !m_candidate.isEmpty(); }
    QVariantList previewWidgets() const;
    QString previewSummary() const { return m_summary; }
    QString designerContextJson() const { return m_designerContext; }
    QString designerMessage() const { return m_designerMessage; }
    int generation() const { return m_generation; }
    bool storageReady() const { return m_storageReady; }
    QString lastError() const { return m_error; }
    Q_INVOKABLE bool addHumidityCard();
    Q_INVOKABLE bool rollback();
    Q_INVOKABLE QVariantList historyEntries();
    Q_INVOKABLE bool previewRestoreGeneration(int generation);
    Q_INVOKABLE bool resetDemo();
    Q_INVOKABLE bool previewPaletteDemo();
    Q_INVOKABLE bool previewTaskManagerDemo();
    Q_INVOKABLE bool updateWidgetProperty(const QString &id, const QString &path, const QVariant &value);
    Q_INVOKABLE bool moveInto(const QString &id, const QString &parent);
    Q_INVOKABLE bool previewMutation(const QString &json);
    Q_INVOKABLE bool previewDocument(const QString &json);
    Q_INVOKABLE bool addWidget(const QString &kind);
    Q_INVOKABLE bool removeWidget(const QString &id);
    Q_INVOKABLE bool renameWidget(const QString &id, const QString &title);
    Q_INVOKABLE bool moveWidget(const QString &id, const QString &after);
    Q_INVOKABLE bool applyPreview();
    Q_INVOKABLE void cancelPreview();
    Q_INVOKABLE bool prepareDesignerContext(const QString &request);
    Q_INVOKABLE bool previewDesignerResponse(const QString &json);
signals:
    void documentChanged();
    void designerChanged();
private:
    bool loadDocument(const QString &path, QJsonObject &out);
    bool commit(const QJsonObject &newDocument, const QString &description = {});
    bool loadGeneration(int generation, QJsonObject &out, int &parent, QString &parentHash, bool requireMetadata);
    QString metadataPath(int generation) const;
    bool propose(const QJsonObject &operation, const QString &summary);
    bool stage(const QJsonObject &candidate, const QString &summary);
    bool reject(const QString &error);
    QString generationPath(int generation) const;
    QString m_folder;
    QString m_error;
    int m_generation = 0;
    int m_parentGeneration = 0; // Confirmed by the current generation metadata; 0 for legacy/unproven.
    QString m_parentSha256;
    QJsonObject m_document;
    QJsonObject m_candidate;
    QString m_summary;
    bool m_storageReady = true;
    QString m_designerContext;
    QString m_designerMessage;
    int m_contextGeneration = 0;
};
