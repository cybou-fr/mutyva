#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QVariantList>
#include <functional>

class AppDocument;

class WeatherService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hasCurrent READ hasCurrent NOTIFY changed)
    Q_PROPERTY(QString city READ city NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(double temperature READ temperature NOTIFY changed)
    Q_PROPERTY(int humidity READ humidity NOTIFY changed)
    Q_PROPERTY(double wind READ wind NOTIFY changed)
    Q_PROPERTY(int aqi READ aqi NOTIFY changed)
    Q_PROPERTY(QVariantList daily READ daily NOTIFY changed)
public:
    explicit WeatherService(QObject *parent = nullptr, QNetworkAccessManager *network = nullptr);
    void followDocument(AppDocument *document);
    bool hasCurrent() const { return m_hasCurrent; }
    QString city() const { return m_city; }
    QString status() const { return m_status; }
    double temperature() const { return m_temp; }
    int humidity() const { return m_humidity; }
    double wind() const { return m_wind; }
    int aqi() const { return m_aqi; }
    QVariantList daily() const { return m_daily; }
    Q_INVOKABLE void lookup(const QString &city);
signals:
    void changed();
private:
    void getJson(const QUrl &url, std::function<void(const QJsonObject &)> onSuccess);
    void loadWeather(double lat, double lon);
    void loadAir(double lat, double lon);
    QNetworkAccessManager m_ownedNetwork;
    QNetworkAccessManager *m_network;
    QMetaObject::Connection m_documentConnection;
    QString m_configuredCity;
    quint64 m_lookupRevision = 0;
    bool m_hasCurrent = false;
    QString m_city = "—";
    QString m_status = "Ожидание данных";
    double m_temp = 0.0;
    int m_humidity = 0;
    double m_wind = 0.0;
    int m_aqi = -1;
    QVariantList m_daily;
};
