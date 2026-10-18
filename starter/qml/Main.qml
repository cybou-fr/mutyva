import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1020
    height: 760
    visible: true
    title: "MUTYVA — City Pulse"
    color: "#101625"
    property color fg: "#f0f5ff"
    property color muted: "#a3b0c8"

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
            width: parent.width - 40
            x: 20
            spacing: 18
            Item { Layout.preferredHeight: 12 }
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Rectangle {
                    width: 39; height: 39; radius: 12; color: "#2864b1"
                    Text { anchors.centerIn: parent; text: "M"; color: "white"; font.bold: true; font.pixelSize: 22 }
                }
                ColumnLayout {
                    spacing: 1
                    Text { text: "MUTYVA"; font.bold: true; font.pixelSize: 22; color: root.fg }
                    Text { text: "City Pulse · public API demo"; color: root.muted; font.pixelSize: 12 }
                }
                Item { Layout.fillWidth: true }
                Label { text: "Поколение " + appDocument.generation; color: root.muted }
            }
            Text {
                text: "Одно приложение. Любая форма."
                color: root.fg; font.pixelSize: root.width < 570 ? 21 : 30
                font.bold: true; wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Text {
                text: "Реальные данные Open-Meteo. Интерфейс собран из локальной JSON-декларации."
                color: root.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true
            }
            Flow {
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height
                spacing: 8
                Button { text: "+ Виджет влажности"; onClicked: appDocument.addHumidityCard() }
                Button { text: "Откат поколения"; onClicked: appDocument.rollback() }
                Button { text: "Сбросить интерфейс"; onClicked: appDocument.resetDemo() }
            }
            Text {
                text: appDocument.lastError.length ? appDocument.lastError : weatherService.status
                color: appDocument.lastError.length ? "#ffaeae" : root.muted
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Repeater {
                model: appDocument.widgets
                delegate: WidgetHost {
                    required property var modelData
                    definition: modelData
                    Layout.fillWidth: true
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#26344d" }
            Text {
                text: "MUTYVA 0.1 proof of concept · Без AI и OAuth в этом стартере. Auth и AI Mutation Engine описаны в документации.\nДанные Open-Meteo · CC BY 4.0 · Некоммерческая демонстрация."
                color: root.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Item { Layout.preferredHeight: 20 }
        }
    }
}
