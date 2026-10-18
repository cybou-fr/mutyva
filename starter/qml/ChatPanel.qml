import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: panel
    objectName: "aiChat"
    edge: Qt.RightEdge
    width: Math.min(parent.width, 460)
    height: parent.height
    property string retryText: {
        var messages = aiChat.messages
        if (!messages.length) return ""
        var last = messages[messages.length - 1]
        return last.role === "user" && last.unfinished ? last.text : ""
    }
    modal: true
    leftPadding: 20; rightPadding: 20; topPadding: 20; bottomPadding: 20
    onOpened: composer.forceActiveFocus()
    Connections {
        target: aiChat
        function onProposalReady() { panel.close() }
    }
    background: Rectangle { color: brandIdentity.colors.surface }
    contentItem: ScrollView {
        clip: true
        contentWidth: availableWidth
        ColumnLayout {
            width: parent.width
            spacing: 16
            RowLayout {
                Layout.fillWidth: true
                Label { text: "Чат с AI"; font.pixelSize: 22; font.bold: true; Layout.fillWidth: true }
                Button { text: "Закрыть"; onClicked: panel.close() }
            }
            Label {
                text: "Опишите, что вы хотите сделать"
                font.pixelSize: 18; font.bold: true
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Label {
                text: "Например: «Мне нужен список задач на день» или «Добавь прогноз погоды»."
                color: brandIdentity.colors.muted
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Label {
                objectName: "aiConnectionStatus"
                text: chatSession.status
                color: brandIdentity.colors.cyan
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                Button {
                    objectName: "connectChatGPT"
                    text: "Continue with ChatGPT"
                    visible: !chatSession.connected && !chatSession.busy
                    onClicked: chatSession.signIn()
                }
                Button { text: "Отменить вход"; visible: chatSession.busy; onClicked: chatSession.cancel() }
                Button { text: "Выйти"; visible: chatSession.connected; enabled: !aiChat.busy; onClicked: chatSession.signOut() }
            }
            Label {
                text: "При отправке ChatGPT получит ваш запрос, недавние сообщения и описание интерфейса. Заметки и значения полей не передаются. История чата и вход сохраняются только до закрытия приложения."
                font.pixelSize: 12; color: brandIdentity.colors.muted
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            ListView {
                id: conversation
                objectName: "chatMessages"
                visible: count > 0
                Layout.fillWidth: true; Layout.preferredHeight: visible ? Math.max(120, panel.height * 0.22) : 0
                clip: true; spacing: 12
                model: aiChat.messages
                onCountChanged: Qt.callLater(function() { conversation.positionViewAtEnd() })
                delegate: Label {
                    required property var modelData
                    width: conversation.width
                    text: (modelData.role === "user" ? "Вы\n" : "MUTYVA\n") + modelData.text + (modelData.unfinished ? "\nЗапрос не завершён" : "")
                    textFormat: Text.PlainText; wrapMode: Text.WordWrap
                    padding: 10
                    background: Rectangle { radius: 10; color: brandIdentity.colors.background }
                }
                ScrollBar.vertical: ScrollBar {}
            }
            Label {
                text: aiChat.status
                visible: text.length > 0
                textFormat: Text.PlainText
                color: brandIdentity.colors.cyan
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Button {
                text: "Повторить запрос"
                visible: panel.retryText.length > 0 && chatSession.connected && !aiChat.busy
                enabled: !appDocument.hasPreview
                onClicked: aiChat.send(panel.retryText)
            }
            Button { text: "Посмотреть изменение"; visible: appDocument.hasPreview; onClicked: panel.close() }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(140, panel.height * 0.2)
                contentWidth: availableWidth
                TextArea {
                    id: composer
                    objectName: "chatComposer"
                    placeholderText: "Какое приложение вам нужно?"
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    Accessible.name: "Сообщение AI"
                    enabled: !aiChat.busy
                    Keys.onPressed: function(event) {
                        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers & Qt.ControlModifier)) {
                            sendButton.clicked(); event.accepted = true
                        }
                    }
                }
            }
            Button {
                id: sendButton
                objectName: "sendChatMessage"
                text: aiChat.busy ? "Отменить запрос" : "Отправить"
                enabled: aiChat.busy || (chatSession.connected && !appDocument.hasPreview && composer.text.trim().length > 0)
                onClicked: {
                    if (aiChat.busy) aiChat.cancel()
                    else if (aiChat.send(composer.text)) composer.clear()
                }
                Layout.alignment: Qt.AlignRight
            }
        }
    }
}
