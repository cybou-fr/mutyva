import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1020
    height: 760
    visible: true
    title: "MUTYVA"
    color: brandIdentity.colors.background
    property color fg: brandIdentity.colors.foreground
    property color muted: brandIdentity.colors.muted
    property bool editorOpen: false
    palette.window: brandIdentity.colors.background
    palette.windowText: root.fg
    palette.text: root.fg
    palette.base: brandIdentity.colors.surface
    palette.button: brandIdentity.colors.button
    palette.buttonText: root.fg
    palette.highlight: brandIdentity.colors.violet
    palette.highlightedText: "white"
    palette.placeholderText: root.muted
    IdentityDialog { id: welcomeDialog; welcomeMode: true }
    IdentityDialog { id: aboutDialog }
    Component.onCompleted: { if (brandIdentity.welcomeRequired) welcomeDialog.open() }

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
                Image {
                    objectName: "brandHeaderIcon"
                    Layout.preferredWidth: 48; Layout.preferredHeight: 48
                    source: brandIdentity.iconUrl; fillMode: Image.PreserveAspectFit; mipmap: true
                }
                ColumnLayout {
                    spacing: 1
                    Text { text: "MUTYVA"; font.bold: true; font.pixelSize: 22; font.letterSpacing: 2; color: root.fg }
                    Text { text: "One App. Infinite Forms."; color: brandIdentity.colors.cyan; font.pixelSize: 12 }
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
                Button { objectName: "openAbout"; text: "О MUTYVA"; onClicked: aboutDialog.open() }
                Button {
                    objectName: "reloadData"; text: "Перечитать данные"
                    onClicked: interactionState.reload()
                    ToolTip.visible: hovered
                    ToolTip.text: "SQLite: " + interactionState.storagePath
                }
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
                Label { text: "Предпросмотр · ещё не сохранено"; color: brandIdentity.colors.cyan; font.bold: true; font.pixelSize: 18 }
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
