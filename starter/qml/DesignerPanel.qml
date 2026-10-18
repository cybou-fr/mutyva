import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: panel
    spacing: 10
    property var selectedWidget: widgetPicker.currentIndex >= 0
                                 ? (appDocument.widgets[widgetPicker.currentIndex] || null) : null
    property var placements: [{id: "", title: "В начало"}].concat(
        appDocument.widgets.map(function(w) { return {id: w.id, title: "После: " + w.id} }))

    Label { text: "Редактор интерфейса"; color: "white"; font.bold: true; font.pixelSize: 18 }
    Label {
        text: "Выберите изменение, проверьте результат ниже и нажмите «Применить»."
        color: "#a3b0c8"; wrapMode: Text.WordWrap; Layout.fillWidth: true
    }
    Flow {
        Layout.fillWidth: true
        Layout.preferredHeight: childrenRect.height
        spacing: 8
        ComboBox {
            id: kindPicker
            width: Math.min(280, panel.width)
            model: appDocument.catalog
            textRole: "title"
        }
        Button {
            objectName: "addWidget"
            text: "Добавить компонент"
            enabled: !appDocument.hasPreview
            onClicked: appDocument.addWidget(appDocument.catalog[kindPicker.currentIndex].kind)
        }
    }
    Flow {
        Layout.fillWidth: true
        Layout.preferredHeight: childrenRect.height
        spacing: 8
        ComboBox {
            id: widgetPicker
            objectName: "widgetPicker"
            width: Math.min(280, panel.width)
            model: appDocument.widgets
            textRole: "id"
            onActivated: titleInput.text = panel.selectedWidget ? (panel.selectedWidget.title || "") : ""
        }
        TextField {
            id: titleInput
            objectName: "titleInput"
            width: Math.min(280, panel.width)
            text: panel.selectedWidget ? (panel.selectedWidget.title || "") : ""
            placeholderText: "Заголовок виджета"
            maximumLength: 120
            Accessible.name: "Новый заголовок виджета"
        }
        Button {
            objectName: "renameWidget"
            text: "Изменить заголовок"
            enabled: panel.selectedWidget !== null && !appDocument.hasPreview
            onClicked: appDocument.renameWidget(panel.selectedWidget.id, titleInput.text)
        }
        Button {
            objectName: "removeWidget"
            text: "Удалить"
            enabled: panel.selectedWidget !== null && !appDocument.hasPreview
            onClicked: appDocument.removeWidget(panel.selectedWidget.id)
        }
    }
    Flow {
        Layout.fillWidth: true
        Layout.preferredHeight: childrenRect.height
        spacing: 8
        ComboBox {
            id: afterPicker
            objectName: "afterPicker"
            width: Math.min(280, panel.width)
            model: panel.placements
            textRole: "title"
        }
        Button {
            objectName: "moveWidget"
            text: "Переместить выбранный"
            enabled: panel.selectedWidget !== null && !appDocument.hasPreview
            onClicked: appDocument.moveWidget(panel.selectedWidget.id, panel.placements[afterPicker.currentIndex].id)
        }
    }
    CheckBox { id: jsonToggle; text: "Редактор JSON" }
    ColumnLayout {
        visible: jsonToggle.checked
        Layout.fillWidth: true
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 8
            ComboBox { id: jsonMode; model: ["Декларация приложения", "Мутация"] }
            Button {
                text: "Текущий документ"
                onClicked: { jsonMode.currentIndex = 0; jsonInput.text = appDocument.documentJson }
            }
            Button {
                text: "Пример мутации"
                onClicked: {
                    jsonMode.currentIndex = 1
                    jsonInput.text = JSON.stringify({schemaVersion: 1,
                        appId: JSON.parse(appDocument.documentJson).id,
                        intent: "Добавить карточку влажности после прогноза",
                        operations: [{op: "addWidget", widget: {id: "humidity", kind: "humidity-card",
                            title: "Влажность воздуха"}, after: "forecast"}]}, null, 2)
                }
            }
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 240
            contentWidth: availableWidth
            TextArea {
                id: jsonInput
                objectName: "jsonInput"
                text: appDocument.documentJson
                wrapMode: TextEdit.WrapAnywhere
                font.family: "monospace"
                selectByMouse: true
                Accessible.name: "JSON декларации или мутации"
            }
        }
        Button {
            text: "Проверить и показать"
            enabled: !appDocument.hasPreview
            onClicked: jsonMode.currentIndex === 0 ? appDocument.previewDocument(jsonInput.text)
                                                   : appDocument.previewMutation(jsonInput.text)
        }
    }
}
