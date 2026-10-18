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
    property bool editorOpen: false
    palette.window: "#101625"
    palette.windowText: "#f0f5ff"
    palette.text: "#f0f5ff"
    palette.base: "#1c2840"
    palette.button: "#26344d"
    palette.buttonText: "#f0f5ff"
    palette.highlight: "#2864b1"
    palette.highlightedText: "white"
    palette.placeholderText: "#a3b0c8"

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
                Button { text: root.editorOpen ? "Скрыть редактор" : "Изменить интерфейс"; onClicked: root.editorOpen = !root.editorOpen }
                Button { text: "Пример: заметки"; enabled: !appDocument.hasPreview; onClicked: appDocument.previewPaletteDemo() }
                Button { text: "+ Виджет влажности"; enabled: !appDocument.hasPreview; onClicked: appDocument.addHumidityCard() }
                Button { text: "Предыдущее сохранение"; enabled: !appDocument.hasPreview && appDocument.generation > 1; onClicked: appDocument.rollback() }
                Button { text: "Сбросить интерфейс"; enabled: !appDocument.hasPreview; onClicked: appDocument.resetDemo() }
            }
            DesignerPanel { visible: root.editorOpen; Layout.fillWidth: true }
            Text {
                text: appDocument.lastError.length ? appDocument.lastError : interactionState.lastError.length ? interactionState.lastError : weatherService.status
                color: appDocument.lastError.length || interactionState.lastError.length ? "#ffaeae" : root.muted
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            ColumnLayout {
                visible: appDocument.hasPreview
                Layout.fillWidth: true
                Label { text: "Предпросмотр · ещё не сохранено"; color: "#9befdc"; font.bold: true; font.pixelSize: 18 }
                Label {
                    text: appDocument.previewSummary
                    textFormat: Text.PlainText
                    color: root.fg; wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                Label {
                    text: "Ниже показан будущий интерфейс. Источники данных и разрешения сохранены."
                    color: root.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                RowLayout {
                    Button { objectName: "applyMutation"; text: "Применить"; onClicked: appDocument.applyPreview() }
                    Button { objectName: "cancelMutation"; text: "Отмена"; onClicked: appDocument.cancelPreview() }
                }
            }
            Repeater {
                objectName: "widgetRepeater"
                model: (appDocument.hasPreview ? appDocument.previewWidgets : appDocument.widgets).filter(function(w) { return !w.parent })
                delegate: WidgetHost {
                    required property var modelData
                    definition: modelData
                    Layout.fillWidth: true
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#26344d" }
            Text {
                text: "MUTYVA · Локальный редактор, проверка деклараций и предпросмотр мутаций. AI и OAuth — следующие этапы.\nДанные Open-Meteo · CC BY 4.0 · Некоммерческая демонстрация."
                color: root.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Item { Layout.preferredHeight: 20 }
        }
    }
}
