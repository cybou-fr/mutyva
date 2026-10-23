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
    ChatPanel { id: chatPanel; parent: Overlay.overlay }
    Dialog {
        id: historyDialog
        objectName: "verifiedHistoryDialog"
        title: "История проверенных изменений"
        modal: true
        standardButtons: Dialog.Close
        anchors.centerIn: Overlay.overlay
        width: Math.min(root.width - 24, 560)
        height: Math.min(root.height - 32, 480)
        property var entries: []
        onAboutToShow: entries = appDocument.historyEntries()
        contentItem: ScrollView {
            clip: true
            ColumnLayout {
                width: historyDialog.availableWidth
                spacing: 9
                Label {
                    text: "Восстановление создаст новое поколение интерфейса. Записи SQLite не откатываются."
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Label {
                    visible: historyDialog.entries.length === 0
                    text: "Подтверждённая история недоступна. Проверьте сообщение об ошибке."
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Repeater {
                    model: historyDialog.entries
                    delegate: Frame {
                        id: historyRow
                        required property var modelData
                        Layout.fillWidth: true
                        ColumnLayout {
                            width: parent.width
                            spacing: 4
                            Label {
                                text: "Поколение " + historyRow.modelData.generation
                                      + (historyRow.modelData.current ? " (текущее)" : "")
                                font.bold: true
                                Layout.fillWidth: true
                                textFormat: Text.PlainText
                            }
                            Label {
                                text: historyRow.modelData.description || ""
                                textFormat: Text.PlainText
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            Label {
                                visible: !!historyRow.modelData.timestampUtc
                                text: historyRow.modelData.timestampUtc || ""
                                color: root.muted
                                Layout.fillWidth: true
                            }
                            Button {
                                objectName: "restoreGeneration-" + historyRow.modelData.generation
                                visible: !historyRow.modelData.current
                                enabled: !appDocument.hasPreview
                                text: "Предпросмотр этой версии"
                                onClicked: {
                                    if (appDocument.previewRestoreGeneration(historyRow.modelData.generation))
                                        historyDialog.close()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    Shortcut { sequence: "Ctrl+Shift+D"; onActivated: root.editorOpen = !root.editorOpen }
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
                Button { objectName: "openChat"; text: "Чат с AI"; onClicked: chatPanel.open() }
            }
            Text {
                text: "Одно приложение. Любая форма."
                color: root.fg; font.pixelSize: root.width < 570 ? 21 : 30
                font.bold: true; wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Text {
                text: "Откройте чат и расскажите, какое приложение вам нужно."
                color: root.muted; wrapMode: Text.WordWrap; Layout.fillWidth: true
            }
            Flow {
                Layout.fillWidth: true
                Layout.preferredHeight: childrenRect.height
                spacing: 8
                Button { objectName: "openAbout"; text: "О MUTYVA"; onClicked: aboutDialog.open() }
                Button {
                    objectName: "openHistory"
                    text: "История"
                    enabled: !appDocument.hasPreview && appDocument.generation > 0
                    onClicked: historyDialog.open()
                    ToolTip.visible: hovered
                    ToolTip.text: "Просмотреть и восстановить только подтверждённые поколения"
                }
                Button {
                    visible: root.editorOpen
                    objectName: "reloadData"; text: "Перечитать данные"
                    onClicked: interactionState.reload()
                    ToolTip.visible: hovered
                    ToolTip.text: "Загрузить сохранённые данные заново"
                }
                Button { visible: root.editorOpen; text: root.editorOpen ? "Скрыть редактор" : "Изменить интерфейс"; onClicked: root.editorOpen = !root.editorOpen }
                Button { visible: root.editorOpen; text: "Пример: заметки"; enabled: !appDocument.hasPreview; onClicked: appDocument.previewPaletteDemo() }
                Button { visible: root.editorOpen; text: "Пример: задачи"; enabled: !appDocument.hasPreview; onClicked: appDocument.previewTaskManagerDemo() }
                Button { visible: root.editorOpen; text: "+ Виджет влажности"; enabled: !appDocument.hasPreview; onClicked: appDocument.addHumidityCard() }
                Button { visible: root.editorOpen; text: "Предыдущее сохранение"; enabled: !appDocument.hasPreview && appDocument.generation > 1; onClicked: appDocument.rollback() }
                Button { visible: root.editorOpen; text: "Сбросить интерфейс"; enabled: !appDocument.hasPreview; onClicked: appDocument.resetDemo() }
            }
            Label {
                visible: root.editorOpen && aiChat.diagnostics.length > 0
                text: aiChat.diagnostics; textFormat: Text.PlainText
                Layout.fillWidth: true; wrapMode: Text.WordWrap; color: root.muted
            }
            DesignerPanel { visible: root.editorOpen; Layout.fillWidth: true }
            Text {
                text: root.editorOpen ? (appDocument.lastError.length ? appDocument.lastError : interactionState.lastError.length ? interactionState.lastError : weatherService.status) : (!interactionState.ready ? "Не удалось загрузить ваши данные. Попробуйте ещё раз." : appDocument.lastError.length ? "Не удалось применить изменение. Попробуйте ещё раз." : weatherService.status)
                color: appDocument.lastError.length || interactionState.lastError.length ? "#ffaeae" : root.muted
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Button {
                text: "Повторить загрузку данных"
                visible: !interactionState.ready
                onClicked: interactionState.reload()
            }
            ColumnLayout {
                visible: appDocument.hasPreview
                Layout.fillWidth: true
                Label { text: "Предпросмотр · ещё не сохранено"; color: brandIdentity.colors.cyan; font.bold: true; font.pixelSize: 18 }
                Label {
                    objectName: "semanticPreview"
                    text: appDocument.previewSummary
                    textFormat: Text.PlainText
                    color: root.fg; wrapMode: Text.WordWrap; Layout.fillWidth: true
                }
                Label {
                    text: "Изменения сохранятся после подтверждения."
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
                text: "Данные Open-Meteo · CC BY 4.0 · Некоммерческая демонстрация."
                color: root.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Item { Layout.preferredHeight: 20 }
        }
    }
}
