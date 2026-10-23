import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    property var definition: ({})
    property var widgets: []
    property var widgetStyle: root.definition.style || ({})
    property string tone: widgetStyle.tone || "neutral"
    property color toneColor: tone === "danger" ? "#b55050" : tone === "accent" ? brandIdentity.colors.cyan : brandIdentity.colors.foreground
    implicitHeight: content.item ? content.item.implicitHeight : 1
    Loader {
        id: content
        width: parent.width
        sourceComponent: {
            switch (root.definition.kind) {
            case "text": return textView
            case "card": case "stack": case "row": case "tabs": return containerView
            case "button": return buttonView
            case "input": return inputView
            case "list": return listView
            case "checkbox": return checkboxView
            case "select": return selectView
            case "date-picker": return dateView
            case "dialog": return dialogView
            case "image": return imageView
            case "icon": return iconView
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
                color: root.toneColor
                objectName: "text-" + root.definition.id
                text: root.definition.text || ""; textFormat: Text.PlainText
                Layout.fillWidth: true; wrapMode: Text.WrapAnywhere
            }
        }
    }
    Component {
        id: containerView
        Rectangle {
            color: root.definition.kind === "card" ? (root.tone === "accent" ? "#14465b" : root.tone === "danger" ? "#552b36" : brandIdentity.colors.surface) : "transparent"
            radius: 16
            property int inset: root.widgetStyle.padding !== undefined ? root.widgetStyle.padding : (root.definition.kind === "card" ? 18 : 0)
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
                TabBar {
                    id: tabBar
                    Layout.fillWidth: true
                    visible: root.definition.kind === "tabs"
                    Repeater {
                        model: root.widgets.filter(function(w) { return w.parent === root.definition.id })
                        delegate: TabButton { required property var modelData; text: modelData.title || modelData.id }
                    }
                }
                StackLayout {
                    id: tabPages
                    Layout.fillWidth: true
                    visible: root.definition.kind === "tabs"
                    currentIndex: tabBar.currentIndex
                    Repeater {
                        model: root.definition.kind === "tabs" ? root.widgets.filter(function(w) { return w.parent === root.definition.id }) : []
                        delegate: Loader {
                            id: pageLoader
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: item ? item.implicitHeight : 1
                            source: Qt.resolvedUrl("WidgetHost.qml")
                            onLoaded: item.definition = Qt.binding(function() { return pageLoader.modelData })
                        }
                    }
                }
                GridLayout {
                    id: grid
                    visible: root.definition.kind !== "tabs"
                    property var childrenDefinitions: root.definition.kind === "tabs" ? [] : root.widgets.filter(function(w) { return w.parent === root.definition.id })
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
            enabled: !appDocument.hasPreview && interactionState.ready && root.definition.action !== undefined
            onClicked: interactionState.trigger(root.definition.id)
            background: Rectangle {
                radius: 8
                color: root.tone === "danger" ? "#913d46" : root.tone === "accent" ? "#285975" : brandIdentity.colors.button
            }
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
                enabled: !appDocument.hasPreview && interactionState.ready
                onTextEdited: {
                    if (!interactionState.setValue(root.definition.id, text))
                        text = Qt.binding(function() { var revision = interactionState.revision; return interactionState.value(root.definition.id, root.definition.value || "") })
                }
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

    Component {
        id: checkboxView
        CheckBox {
            text: root.definition.title || "Флажок"
            enabled: !appDocument.hasPreview && interactionState.ready
            checked: {
                var revision = interactionState.revision
                return (appDocument.hasPreview ? (root.definition.value || "false")
                    : interactionState.value(root.definition.id, root.definition.value || "false")) === "true"
            }
            onToggled: interactionState.setValue(root.definition.id, checked ? "true" : "false")
        }
    }
    Component {
        id: selectView
        ColumnLayout {
            Label { text: root.definition.title || "Выбор"; textFormat: Text.PlainText; Layout.fillWidth: true }
            ComboBox {
                Layout.fillWidth: true
                model: root.definition.options || []
                enabled: !appDocument.hasPreview && interactionState.ready
                currentIndex: {
                    var revision = interactionState.revision
                    var value = appDocument.hasPreview ? (root.definition.value || "")
                        : interactionState.value(root.definition.id, root.definition.value || "")
                    return Math.max(0, (root.definition.options || []).indexOf(value))
                }
                onActivated: interactionState.setValue(root.definition.id, currentText)
            }
        }
    }
    Component {
        id: dateView
        ColumnLayout {
            spacing: 5
            Label { text: root.definition.title || "Дата"; textFormat: Text.PlainText; Layout.fillWidth: true }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: dateInput
                    Layout.fillWidth: true
                    placeholderText: "ГГГГ-ММ-ДД"
                    enabled: !appDocument.hasPreview && interactionState.ready
                    text: {
                        var revision = interactionState.revision
                        return appDocument.hasPreview ? (root.definition.value || "")
                            : interactionState.value(root.definition.id, root.definition.value || "")
                    }
                    onEditingFinished: interactionState.setValue(root.definition.id, text)
                }
                Button {
                    text: "Календарь"
                    enabled: !appDocument.hasPreview && interactionState.ready
                    onClicked: datePopup.open()
                }
                Popup {
                    id: datePopup
                    modal: true
                    width: Math.min(340, root.width); height: 320
                    property int shownYear: new Date().getFullYear()
                    property int shownMonth: new Date().getMonth()
                    ColumnLayout {
                        anchors.fill: parent
                        RowLayout {
                            Button {
                                text: "‹"
                                onClicked: { if (--datePopup.shownMonth < 0) { datePopup.shownMonth = 11; --datePopup.shownYear } }
                            }
                            Label { text: datePopup.shownYear + "-" + (datePopup.shownMonth + 1); Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter }
                            Button {
                                text: "›"
                                onClicked: { if (++datePopup.shownMonth > 11) { datePopup.shownMonth = 0; ++datePopup.shownYear } }
                            }
                        }
                        MonthGrid {
                            Layout.fillWidth: true; Layout.fillHeight: true
                            year: datePopup.shownYear; month: datePopup.shownMonth
                            onClicked: function(date) {
                                interactionState.setValue(root.definition.id, Qt.formatDate(date, "yyyy-MM-dd"))
                                datePopup.close()
                            }
                        }
                    }
                }
            }
        }
    }
    Component {
        id: dialogView
        Item {
            implicitHeight: trigger.implicitHeight
            Button {
                id: trigger
                text: root.definition.title || "Показать"
                onClicked: infoDialog.open()
                enabled: !appDocument.hasPreview
            }
            Dialog {
                id: infoDialog
                title: root.definition.title || "Информация"
                modal: true
                standardButtons: Dialog.Ok
                anchors.centerIn: Overlay.overlay
                Label {
                    text: root.definition.message || ""
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                    width: Math.min(350, root.width)
                }
            }
        }
    }
    Component {
        id: imageView
        ColumnLayout {
            Image {
                Layout.fillWidth: true; Layout.preferredHeight: 130
                fillMode: Image.PreserveAspectFit
                source: {
                    switch (root.definition.source) {
                        case "mutyva-logo": return "qrc:/branding/exports/mutyva-logo.png"
                        case "mutyva-icon": return "qrc:/branding/exports/mutyva-icon.png"
                        default: return "qrc:/branding/exports/mutyva-symbol.png"
                    }
                }
            }
            Label {
                visible: !!root.definition.title
                text: root.definition.title || ""
                textFormat: Text.PlainText; Layout.fillWidth: true
            }
        }
    }
    Component {
        id: iconView
        Label {
            text: {
                switch(root.definition.icon) {
                    case "check": return "✓"
                    case "heart": return "♥"
                    case "info": return "ⓘ"
                    case "warning": return "⚠"
                    case "folder": return "▣"
                    case "calendar": return "▦"
                    default: return "★"
                }
            }
            color: root.toneColor
            font.pixelSize: 28
            horizontalAlignment: Text.AlignHCenter
            Accessible.name: root.definition.title || root.definition.icon || "Иконка"
        }
    }
}
