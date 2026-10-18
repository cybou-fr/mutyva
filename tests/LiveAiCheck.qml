import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    visible: true; width: 660; height: 480
    title: "MUTYVA — сквозная проверка ChatGPT"
    color: "#0B1125"
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 24; spacing: 16
        Label { text: "Проверка реального подключения"; color: "white"; font.pixelSize: 22; font.bold: true; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Label {
            text: "Завершите вход в открытом браузере. Дальше тест сам отправит три запроса и проверит ответы, предпросмотр и восстановление. Ваше приложение и заметки не изменятся."
            color: "#A3B0C8"; Layout.fillWidth: true; wrapMode: Text.WordWrap
        }
        Label { text: chatSession.status; color: "#00D4FF"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Label { text: liveCheck.status; color: "white"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        BusyIndicator { running: !liveCheck.finished }
        Repeater {
            model: liveCheck.results
            Label { required property var modelData; text: (modelData.passed ? "✓ " : "✗ ") + modelData.name; color: modelData.passed ? "#7BE3B2" : "#FFAEAE" }
        }
        Item { Layout.fillHeight: true }
        Button { text: "Отменить проверку"; enabled: !liveCheck.finished; onClicked: liveCheck.cancel() }
    }
}
