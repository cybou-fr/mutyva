import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var definition: ({})
    property var widgets: []
    implicitHeight: content.item ? content.item.implicitHeight : 1
    Loader {
        id: content
        width: parent.width
        sourceComponent: {
            switch (root.definition.kind) {
            case "text": return textView
            case "card": case "stack": case "row": return containerView
            case "button": return buttonView
            case "input": return inputView
            case "list": return listView
            }
            return null
        }
    }
    Component {
        id: textView
        ColumnLayout {
            spacing: 6
            Label {
                text: root.definition.title || ""; visible: text.length > 0
                textFormat: Text.PlainText; font.bold: true
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Label {
                objectName: "text-" + root.definition.id
                text: root.definition.text || ""; textFormat: Text.PlainText
                Layout.fillWidth: true; wrapMode: Text.WrapAnywhere
            }
        }
    }
    Component {
        id: containerView
        Rectangle {
            color: root.definition.kind === "card" ? brandIdentity.colors.surface : "transparent"
            radius: 16
            property int inset: root.definition.kind === "card" ? 18 : 0
            implicitHeight: body.implicitHeight + inset * 2
            ColumnLayout {
                id: body
                x: parent.inset; y: parent.inset
                width: parent.width - parent.inset * 2
                spacing: 12
                Label {
                    text: root.definition.title || ""; visible: text.length > 0
                    textFormat: Text.PlainText; font.bold: true
                    Layout.fillWidth: true; wrapMode: Text.WordWrap
                }
                GridLayout {
                    id: grid
                    property var childrenDefinitions: root.widgets.filter(function(w) { return w.parent === root.definition.id })
                    Layout.fillWidth: true
                    columns: root.definition.kind === "row" && width >= Math.max(500, childrenDefinitions.length * 160) ? Math.max(1, childrenDefinitions.length) : 1
                    columnSpacing: 10; rowSpacing: 12
                    Repeater {
                        model: grid.childrenDefinitions
                        delegate: Loader {
                            id: nested
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.preferredWidth: (grid.width - (grid.columns - 1) * grid.columnSpacing) / grid.columns
                            Layout.preferredHeight: item ? item.implicitHeight : 1
                            source: Qt.resolvedUrl("WidgetHost.qml")
                            onLoaded: item.definition = Qt.binding(function() { return nested.modelData })
                        }
                    }
                }
            }
        }
    }
    Component {
        id: buttonView
        Button {
            id: actionButton
            objectName: "action-" + root.definition.id
            text: root.definition.title === undefined ? "Кнопка" : root.definition.title
            implicitHeight: Math.max(36, contentItem.implicitHeight + topPadding + bottomPadding)
            contentItem: Label {
                text: actionButton.text; textFormat: Text.PlainText
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WrapAnywhere
                color: actionButton.enabled ? brandIdentity.colors.foreground : "#7c8aa3"
            }
            enabled: !appDocument.hasPreview && root.definition.action !== undefined
            onClicked: interactionState.trigger(root.definition.id)
        }
    }
    Component {
        id: inputView
        ColumnLayout {
            spacing: 6
            Label {
                text: root.definition.title || ""; visible: text.length > 0
                textFormat: Text.PlainText; Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            TextField {
                objectName: "input-" + root.definition.id
                Layout.fillWidth: true
                maximumLength: 500
                placeholderText: root.definition.placeholder || ""
                text: { var revision = interactionState.revision; return appDocument.hasPreview ? (root.definition.value || "") : interactionState.value(root.definition.id, root.definition.value || "") }
                enabled: !appDocument.hasPreview
                onTextEdited: interactionState.setValue(root.definition.id, text)
                Accessible.name: root.definition.title || "Поле ввода"
            }
        }
    }
    Component {
        id: listView
        ColumnLayout {
            spacing: 8
            Label {
                text: root.definition.title || ""; visible: text.length > 0
                textFormat: Text.PlainText; font.bold: true
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Repeater {
                id: rows
                model: { var revision = interactionState.revision; return appDocument.hasPreview ? (root.definition.items || []) : interactionState.items(root.definition.id, root.definition.items || []) }
                delegate: Label {
                    required property var modelData
                    text: "• " + String(modelData); textFormat: Text.PlainText
                    Layout.fillWidth: true; wrapMode: Text.WrapAnywhere
                }
            }
            Label { text: "Список пуст"; visible: rows.count === 0; color: brandIdentity.colors.muted }
        }
    }
}
