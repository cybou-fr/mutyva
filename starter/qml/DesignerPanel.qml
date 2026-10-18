import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: panel
    spacing: 10
    property string selectedId: ""
    property var selectedWidget: appDocument.widgets.find(function(w) { return w.id === panel.selectedId })
                                 || appDocument.widgets[0] || null
    property var placements: [{id: "", title: "В начало"}].concat(
        appDocument.widgets.filter(function(w) { return (w.parent || "") === (panel.selectedWidget ? (panel.selectedWidget.parent || "") : "") })
        .map(function(w) { return {id: w.id, title: "После: " + w.id} }))
    property var selectedDescriptor: appDocument.catalog.find(function(c) { return panel.selectedWidget && c.kind === panel.selectedWidget.kind })
    property var propertyNames: selectedDescriptor ? Object.keys(selectedDescriptor.writableProperties).filter(function(p) { return p !== "title" }) : []
    property var parents: [{id: "", title: "Корень интерфейса"}].concat(appDocument.widgets.filter(function(w) {
        return appDocument.catalog.some(function(c) { return c.kind === w.kind && c.container })
    }).map(function(w) { return {id: w.id, title: w.id} }))
    property string propertyError: ""
    function propertyText() {
        if (!selectedWidget || propertyPicker.currentIndex < 0) return ""
        var value = selectedWidget[propertyNames[propertyPicker.currentIndex]]
        if (value === undefined) return ""
        return typeof value === "string" ? value : JSON.stringify(value)
    }

    Label { text: "Редактор интерфейса"; color: "white"; font.bold: true; font.pixelSize: 18 }
    Label {
        text: "Выберите изменение, проверьте результат ниже и нажмите «Применить»."
        color: "#a3b0c8"; wrapMode: Text.WordWrap; Layout.fillWidth: true
    }
    Label {
        text: appDocument.catalog[kindPicker.currentIndex]
              ? appDocument.catalog[kindPicker.currentIndex].description : ""
        color: "#a3b0c8"; Layout.fillWidth: true; wrapMode: Text.WordWrap
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
            onModelChanged: Qt.callLater(function() {
                widgetPicker.currentIndex = panel.selectedWidget
                    ? appDocument.widgets.findIndex(function(w) { return w.id === panel.selectedWidget.id }) : -1
            })
            onActivated: {
                panel.selectedId = appDocument.widgets[currentIndex].id
                titleInput.text = panel.selectedWidget ? (panel.selectedWidget.title || "") : ""
                propertyValue.text = panel.propertyText()
            }
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
        Layout.fillWidth: true; Layout.preferredHeight: childrenRect.height; spacing: 8
        ComboBox {
            id: parentPicker; objectName: "parentPicker"; width: Math.min(280, panel.width)
            model: panel.parents; textRole: "title"
        }
        Button {
            objectName: "moveInto"
            text: "Вложить выбранный компонент"
            enabled: panel.selectedWidget !== null && !appDocument.hasPreview
            onClicked: appDocument.moveInto(panel.selectedWidget.id, panel.parents[parentPicker.currentIndex].id)
        }
    }
    Flow {
        visible: panel.propertyNames.length > 0
        Layout.fillWidth: true; Layout.preferredHeight: implicitHeight; spacing: 8
        ComboBox {
            id: propertyPicker; width: Math.min(180, panel.width)
            model: panel.propertyNames
            onActivated: propertyValue.text = panel.propertyText()
        }
        TextField {
            id: propertyValue; objectName: "propertyValue"; width: Math.min(380, panel.width)
            text: panel.propertyText()
            placeholderText: "Значение; для items и action — JSON"
            Accessible.name: "Значение свойства компонента"
        }
        Button {
            objectName: "updateWidgetProperty"
            text: "Изменить свойство"
            enabled: panel.selectedWidget !== null && !appDocument.hasPreview
            onClicked: {
                panel.propertyError = ""
                var path = panel.propertyNames[propertyPicker.currentIndex]
                try {
                    var value = path === "items" || path === "action" ? JSON.parse(propertyValue.text) : propertyValue.text
                    appDocument.updateWidgetProperty(panel.selectedWidget.id, path, value)
                } catch (error) { panel.propertyError = "Введите корректный JSON для свойства " + path }
            }
        }
    }
    Label { text: panel.propertyError; visible: text.length > 0; color: "#ffaeae"; Layout.fillWidth: true; wrapMode: Text.WordWrap }
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
            ComboBox { id: jsonMode; model: ["Декларация приложения", "Мутация", "Ответ AI"] }
            Button {
                text: "Текущий документ"
                onClicked: { jsonMode.currentIndex = 0; jsonInput.text = appDocument.documentJson }
            }
            Button {
                text: "Пример мутации"
                onClicked: {
                    jsonMode.currentIndex = 1
                    jsonInput.text = JSON.stringify({schemaVersion: 2,
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
            onClicked: {
                if (jsonMode.currentIndex === 0) appDocument.previewDocument(jsonInput.text)
                else if (jsonMode.currentIndex === 1) appDocument.previewMutation(jsonInput.text)
                else appDocument.previewDesignerResponse(jsonInput.text)
            }
        }
    }
    DesignerContextPanel { Layout.fillWidth: true }
}
