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
public:
    explicit AppDocument(QObject *parent = nullptr);
    QVariantList widgets() const;
    int generation() const { return m_generation; }
    QString lastError() const { return m_error; }
    Q_INVOKABLE bool addHumidityCard();
    Q_INVOKABLE bool rollback();
    Q_INVOKABLE bool resetDemo();
signals:
    void documentChanged();
private:
    bool loadDocument(const QString &path, QJsonObject &out);
    bool commit(const QJsonObject &newDocument);
    QString generationPath(int generation) const;
    QString m_folder;
    QString m_error;
    int m_generation = 0;
    QJsonObject m_document;
};
