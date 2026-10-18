import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Loader {
    id: host
    property var definition: ({})
    Layout.fillWidth: true
    Layout.preferredHeight: item ? item.implicitHeight : 1
    sourceComponent: {
        switch (definition.kind) {
        case "city-search": return citySearch;
        case "weather-card": return weatherCard;
        case "forecast-list": return forecastList;
        case "air-card": return airCard;
        case "humidity-card": return humidityCard;
        default: return unknownCard;
        }
    }
    onLoaded: { if (item) item.definition = host.definition }
    Component {
        id: citySearch
        Rectangle {
            property var definition: ({})
            implicitHeight: 96; radius: 16; color: "#1c2840"
            RowLayout {
                anchors.fill: parent; anchors.margins: 16; spacing: 10
                TextField {
                    id: cityInput
                    Layout.fillWidth: true
                    text: "Paris"; placeholderText: "Название города"
                    onAccepted: weatherService.lookup(text)
                }
                Button { text: "Найти"; onClicked: weatherService.lookup(cityInput.text) }
            }
        }
    }
    Component {
        id: weatherCard
        Rectangle {
            property var definition: ({})
            implicitHeight: 165; radius: 16; color: "#1c2840"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 18; spacing: 8
                Text { text: weatherService.city; color: "#a3b0c8"; font.pixelSize: 17 }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: weatherService.temperature.toFixed(1) + " °C"; color: "white"; font.bold: true; font.pixelSize: 39 }
                    Item { Layout.fillWidth: true }
                    Text { text: "Ветер " + weatherService.wind.toFixed(1) + " км/ч"; color: "#a3b0c8"; font.pixelSize: 14 }
                }
            }
        }
    }
    Component {
        id: forecastList
        Rectangle {
            property var definition: ({})
            implicitHeight: daysCol.implicitHeight + 35; radius: 16; color: "#1c2840"
            ColumnLayout {
                id: daysCol
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                anchors.margins: 17; spacing: 9
                Text { text: "Прогноз на 7 дней"; color: "white"; font.bold: true; font.pixelSize: 17 }
                Repeater {
                    model: weatherService.daily
                    delegate: RowLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        Text { text: modelData.date; color: "#b9c6dd"; Layout.preferredWidth: 94; font.pixelSize: 12 }
                        Item { Layout.fillWidth: true }
                        Text { text: Number(modelData.min).toFixed(0) + "…" + Number(modelData.max).toFixed(0) + " °C"; color: "white"; font.pixelSize: 12 }
                        Text { text: "☔ " + Number(modelData.rain).toFixed(1) + " мм"; color: "#a3b0c8"; font.pixelSize: 12 }
                    }
                }
            }
        }
    }
    Component {
        id: airCard
        Rectangle {
            property var definition: ({})
            implicitHeight: 98; radius: 16; color: "#1c2840"
            RowLayout {
                anchors.fill: parent; anchors.margins: 18
                ColumnLayout {
                    Text { text: "Качество воздуха"; color: "#a3b0c8" }
                    Text { text: "European Air Quality Index"; color: "white"; font.pixelSize: 12 }
                }
                Item { Layout.fillWidth: true }
                Text { text: weatherService.aqi >= 0 ? weatherService.aqi.toString() : "—"; color: "#7ed1ac"; font.bold: true; font.pixelSize: 31 }
            }
        }
    }
    Component {
        id: humidityCard
        Rectangle {
            property var definition: ({})
            implicitHeight: 94; radius: 16; color: "#143c47"
            RowLayout {
                anchors.fill: parent; anchors.margins: 18
                Text { text: "Добавлено мутацией · Влажность"; color: "white"; font.pixelSize: 15 }
                Item { Layout.fillWidth: true }
                Text { text: weatherService.humidity + " %"; color: "#9befdc"; font.bold: true; font.pixelSize: 29 }
            }
        }
    }
    Component {
        id: unknownCard
        Rectangle {
            property var definition: ({})
            implicitHeight: 64; radius: 12; color: "#5b2525"
            Text { anchors.centerIn: parent; text: "Неизвестный тип компонента"; color: "white" }
        }
    }
}
