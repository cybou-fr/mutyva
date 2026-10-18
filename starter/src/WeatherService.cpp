#include "WeatherService.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariantMap>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QDebug>

WeatherService::WeatherService(QObject *parent) : QObject(parent) {}

void WeatherService::getJson(const QUrl &url, std::function<void(const QJsonObject &)> onSuccess) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "MUTYVA-CityPulse-Demo/0.1");
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess = std::move(onSuccess)]() {
        const auto error = reply->error();
        const QString errorText = reply->errorString();
        const QByteArray bytes = reply->readAll();
        reply->deleteLater();
        if (error != QNetworkReply::NoError) {
            m_status = "Ошибка сети: " + errorText;
            emit changed();
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            m_status = "Некорректный JSON API";
            emit changed();
            return;
        }
        onSuccess(doc.object());
    });
}

void WeatherService::lookup(const QString &city) {
    const QString clean = city.trimmed().left(100);
    if (clean.size() < 2) { m_status = "Введите минимум 2 символа"; emit changed(); return; }
    m_status = "Поиск города..."; emit changed();
    QUrl url("https://geocoding-api.open-meteo.com/v1/search");
    QUrlQuery q;
    q.addQueryItem("name", clean);
    q.addQueryItem("count", "1");
    q.addQueryItem("language", "ru");
    q.addQueryItem("format", "json");
    url.setQuery(q);
    getJson(url, [this](const QJsonObject &obj) {
        const QJsonArray results = obj.value("results").toArray();
        if (results.isEmpty()) { m_status = "Город не найден"; emit changed(); return; }
        const QJsonObject place = results.at(0).toObject();
        m_city = place.value("name").toString() + ", " + place.value("country").toString();
        m_status = "Загрузка прогноза...";
        m_aqi = -1; m_daily.clear(); emit changed();
        const double lat = place.value("latitude").toDouble();
        const double lon = place.value("longitude").toDouble();
        loadWeather(lat, lon); loadAir(lat, lon);
    });
}

void WeatherService::loadWeather(double lat, double lon) {
    QUrl url("https://api.open-meteo.com/v1/forecast");
    QUrlQuery q;
    q.addQueryItem("latitude", QString::number(lat, 'f', 5));
    q.addQueryItem("longitude", QString::number(lon, 'f', 5));
    q.addQueryItem("current", "temperature_2m,relative_humidity_2m,wind_speed_10m");
    q.addQueryItem("daily", "temperature_2m_max,temperature_2m_min,precipitation_sum");
    q.addQueryItem("timezone", "auto");
    q.addQueryItem("forecast_days", "7");
    url.setQuery(q);
    getJson(url, [this](const QJsonObject &obj) {
        const QJsonObject current = obj.value("current").toObject();
        m_temp = current.value("temperature_2m").toDouble();
        m_humidity = current.value("relative_humidity_2m").toInt();
        m_wind = current.value("wind_speed_10m").toDouble();
        const QJsonObject days = obj.value("daily").toObject();
        const QJsonArray dates = days.value("time").toArray();
        const QJsonArray maxima = days.value("temperature_2m_max").toArray();
        const QJsonArray minima = days.value("temperature_2m_min").toArray();
        const QJsonArray rain = days.value("precipitation_sum").toArray();
        m_daily.clear();
        for (qsizetype i = 0; i < dates.size(); ++i) {
            QVariantMap d;
            d.insert("date", dates.at(i).toString());
            d.insert("max", i < maxima.size() ? maxima.at(i).toDouble() : 0.0);
            d.insert("min", i < minima.size() ? minima.at(i).toDouble() : 0.0);
            d.insert("rain", i < rain.size() ? rain.at(i).toDouble() : 0.0);
            m_daily.append(d);
        }
        m_status = "Данные обновлены"; emit changed();
    });
}

void WeatherService::loadAir(double lat, double lon) {
    QUrl url("https://air-quality-api.open-meteo.com/v1/air-quality");
    QUrlQuery q;
    q.addQueryItem("latitude", QString::number(lat, 'f', 5));
    q.addQueryItem("longitude", QString::number(lon, 'f', 5));
    q.addQueryItem("current", "european_aqi,pm2_5,pm10");
    q.addQueryItem("timezone", "auto");
    url.setQuery(q);
    getJson(url, [this](const QJsonObject &obj) {
        m_aqi = obj.value("current").toObject().value("european_aqi").toInt(-1);
        emit changed();
    });
}
