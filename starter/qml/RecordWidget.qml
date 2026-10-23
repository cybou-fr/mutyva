import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Record controls are compiled trusted Qt components. The document supplies only model IDs
// and typed field descriptors; it cannot inject QML/SQL/JS.
Item {
    id: root
    property var definition: ({})
    property var fields: interactionState.modelFields(definition.model || "")
    property var selected: {
        var rev = interactionState.revision
        return interactionState.selectedRecord(definition.model || "")
    }
    property var records: {
        var rev = interactionState.revision
        return interactionState.records(definition.id || "")
    }
    property var draft: ({})
    property bool canEdit: !appDocument.hasPreview && interactionState.ready
    implicitHeight: content.item ? content.item.implicitHeight : 1

    function resetDraft() {
        var state = {}
        for (var i = 0; i < fields.length; ++i) {
            var field = fields[i]
            var stored = selected && selected._id ? selected[field.id] : undefined
            if (stored === undefined || stored === null)
                stored = field.default !== undefined ? field.default : (field.type === "boolean" ? false : "")
            state[field.id] = stored
        }
        draft = state
    }
    function setDraft(fieldId, value) {
        var next = Object.assign({}, draft)
        next[fieldId] = value
        draft = next
    }
    function submitted() {
        var result = {}
        for (var i = 0; i < fields.length; ++i) {
            var f = fields[i]
            var value = draft[f.id]
            if (f.type === "integer" && value !== "" && value !== undefined && value !== null) {
                if (!/^-?\d+$/.test(String(value))) return
                value = Number(value)
            }
            if (f.type === "boolean") value = !!value
            result[f.id] = value
        }
        var ok = selected && selected._id
            ? interactionState.updateRecord(definition.id, selected._id, result)
            : interactionState.createRecord(definition.id, result)
        if (ok) resetDraft()
    }
    onSelectedChanged: resetDraft()
    onFieldsChanged: resetDraft()
    Component.onCompleted: resetDraft()

    Loader {
        id: content
        width: parent.width
        sourceComponent: root.definition.kind === "record-form" ? formView
                       : root.definition.kind === "record-list" ? listView : null
    }
    Component {
        id: formView
        Rectangle {
            radius: 12
            color: brandIdentity.colors.surface
            implicitHeight: body.implicitHeight + 28
            ColumnLayout {
                id: body
                x: 14; y: 14; width: parent.width - 28; spacing: 10
                Label {
                    text: root.definition.title || "Форма"
                    font.bold: true; Layout.fillWidth: true
                    textFormat: Text.PlainText; wrapMode: Text.WordWrap
                }
                Repeater {
                    model: root.fields
                    delegate: ColumnLayout {
                        id: fieldDelegate
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 3
                        Label {
                            text: fieldDelegate.modelData.label + (fieldDelegate.modelData.required ? " *" : "")
                            textFormat: Text.PlainText; Layout.fillWidth: true; wrapMode: Text.WordWrap
                        }
                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: fieldDelegate.modelData.type === "boolean" ? boolControl : textControl
                        }
                        Component {
                            id: boolControl
                            CheckBox {
                                checked: !!root.draft[fieldDelegate.modelData.id]
                                enabled: root.canEdit
                                onToggled: root.setDraft(fieldDelegate.modelData.id, checked)
                            }
                        }
                        Component {
                            id: textControl
                            TextField {
                                Layout.fillWidth: true
                                enabled: root.canEdit
                                text: String(root.draft[fieldDelegate.modelData.id] === undefined || root.draft[fieldDelegate.modelData.id] === null ? "" : root.draft[fieldDelegate.modelData.id])
                                placeholderText: fieldDelegate.modelData.type === "date" ? "ГГГГ-ММ-ДД" :
                                                 fieldDelegate.modelData.type === "integer" ? "Целое число" : ""
                                maximumLength: fieldDelegate.modelData.type === "text" ? 1000 : 40
                                onTextEdited: root.setDraft(fieldDelegate.modelData.id, text)
                            }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        objectName: "recordSubmit-" + root.definition.id
                        text: root.selected && root.selected._id ? "Сохранить изменения" : "Создать запись"
                        enabled: root.canEdit
                        onClicked: root.submitted()
                    }
                    Button {
                        text: "Новая запись"
                        enabled: root.canEdit && !!(root.selected && root.selected._id)
                        onClicked: interactionState.clearSelectedRecord(root.definition.id)
                        visible: root.selected && !!root.selected._id
                    }
                }
            }
        }
    }
    Component {
        id: listView
        Rectangle {
            radius: 12; color: brandIdentity.colors.surface
            implicitHeight: body.implicitHeight + 28
            ColumnLayout {
                id: body
                x: 14; y: 14; width: parent.width - 28; spacing: 8
                Label { text: root.definition.title || "Записи"; font.bold: true; Layout.fillWidth: true }
                Label { visible: root.records.length === 0; text: "Записей нет"; Layout.fillWidth: true }
                Repeater {
                    model: root.records
                    delegate: ColumnLayout {
                        id: rowItem
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 4
                        Frame {
                            Layout.fillWidth: true
                            background: Rectangle { color: "transparent"; border.color: "#4d647c"; radius: 8 }
                            ColumnLayout {
                                width: parent.width; spacing: 6
                                Repeater {
                                    model: root.fields
                                    delegate: Label {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        textFormat: Text.PlainText; wrapMode: Text.WordWrap
                                        text: modelData.label + ": " + String(rowItem.modelData[modelData.id] !== undefined && rowItem.modelData[modelData.id] !== null
                                            ? rowItem.modelData[modelData.id] : (modelData.default !== undefined ? modelData.default : "—"))
                                    }
                                }
                                RowLayout {
                                    Layout.fillWidth: true
                                    Button {
                                        text: "Редактировать"
                                        enabled: root.canEdit
                                        onClicked: interactionState.selectRecord(root.definition.id, rowItem.modelData._id)
                                    }
                                    Button {
                                        text: "Удалить"
                                        enabled: root.canEdit
                                        onClicked: confirmDelete.open()
                                    }
                                }
                            }
                            Dialog {
                                id: confirmDelete
                                title: "Удалить запись?"
                                modal: true
                                standardButtons: Dialog.Ok | Dialog.Cancel
                                anchors.centerIn: Overlay.overlay
                                onAccepted: interactionState.deleteRecord(root.definition.id, rowItem.modelData._id)
                                Label { text: "Удаление записи необратимо."; textFormat: Text.PlainText }
                            }
                        }
                    }
                }
            }
        }
    }
}
