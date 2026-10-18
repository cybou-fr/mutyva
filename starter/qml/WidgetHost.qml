import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Loader {
    id: host
    property var definition: ({})
    property var documentWidgets: appDocument.hasPreview ? appDocument.previewWidgets : appDocument.widgets
    objectName: "widget-" + (definition.id || "unknown")
    Layout.fillWidth: true
    Layout.preferredHeight: item ? item.implicitHeight : 1
    sourceComponent: {
        switch (definition.kind) {
        case "city-search": return citySearch;
        case "weather-card": return weatherCard;
        case "forecast-list": return forecastList;
        case "air-card": return airCard;
        case "humidity-card": return humidityCard;
        case "text": case "card": case "stack": case "row":
        case "button": case "input": case "list": return generalWidget;
        default: return unknownCard;
        }
    }
    Component {
        id: generalWidget
        GeneralWidget { definition: host.definition; widgets: host.documentWidgets }
    }
    Component {
        id: citySearch
        Rectangle {
            implicitHeight: searchLayout.implicitHeight + 32; radius: 16; color: "#1c2840"
            ColumnLayout {
                id: searchLayout
                anchors.fill: parent; anchors.margins: 16; spacing: 10
                Text {
                    text: host.definition.title === undefined ? "Поиск города" : host.definition.title
                    textFormat: Text.PlainText; color: "white"; Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                RowLayout {
                    Layout.fillWidth: true
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
    }
    Component {
        id: weatherCard
        Rectangle {
            implicitHeight: weatherLayout.implicitHeight + 36; radius: 16; color: "#1c2840"
            ColumnLayout {
                id: weatherLayout
                anchors.fill: parent; anchors.margins: 18; spacing: 8
                Text {
                    text: host.definition.title === undefined ? "Текущая погода" : host.definition.title
                    textFormat: Text.PlainText; color: "white"; Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                Text { text: weatherService.city; color: "#a3b0c8"; font.pixelSize: 17; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: weatherService.temperature.toFixed(1) + " °C"; color: "white"; font.bold: true; font.pixelSize: host.width < 400 ? 30 : 39 }
                    Item { Layout.fillWidth: true }
                    Text { text: "Ветер " + weatherService.wind.toFixed(1) + " км/ч"; color: "#a3b0c8"; font.pixelSize: 14; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                }
            }
        }
    }
    Component {
        id: forecastList
        Rectangle {
            implicitHeight: daysCol.implicitHeight + 35; radius: 16; color: "#1c2840"
            ColumnLayout {
                id: daysCol
                anchors.left: parent.left; anchors.right: parent.right; anchors.top: parent.top
                anchors.margins: 17; spacing: 9
                Text {
                    text: host.definition.title === undefined ? "Прогноз на 7 дней" : host.definition.title
                    textFormat: Text.PlainText; color: "white"; font.bold: true; font.pixelSize: 17
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
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
            implicitHeight: airLayout.implicitHeight + 36; radius: 16; color: "#1c2840"
            RowLayout {
                id: airLayout
                anchors.fill: parent; anchors.margins: 18
                ColumnLayout {
                    Layout.fillWidth: true
                    Text {
                        text: host.definition.title === undefined ? "Качество воздуха" : host.definition.title
                        textFormat: Text.PlainText; color: "#a3b0c8"; Layout.fillWidth: true; wrapMode: Text.WordWrap
                    }
                    Text { text: "European Air Quality Index"; color: "white"; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                }
                Item { Layout.fillWidth: true }
                Text { text: weatherService.aqi >= 0 ? weatherService.aqi.toString() : "—"; color: "#7ed1ac"; font.bold: true; font.pixelSize: 31 }
            }
        }
    }
    Component {
        id: humidityCard
        Rectangle {
            implicitHeight: humidityLayout.implicitHeight + 36; radius: 16; color: "#143c47"
            RowLayout {
                id: humidityLayout
                anchors.fill: parent; anchors.margins: 18
                Text {
                    text: host.definition.title === undefined ? "Влажность воздуха" : host.definition.title
                    textFormat: Text.PlainText; color: "white"; font.pixelSize: 15
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                Item { Layout.fillWidth: true }
                Text { text: weatherService.humidity + " %"; color: "#9befdc"; font.bold: true; font.pixelSize: 29 }
            }
        }
    }
    Component {
        id: unknownCard
        Rectangle {
            implicitHeight: 64; radius: 12; color: "#5b2525"
            Text { anchors.centerIn: parent; text: "Неизвестный тип компонента"; color: "white" }
        }
    }
}
