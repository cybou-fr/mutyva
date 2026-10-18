import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: panel
    objectName: "aiChat"
    edge: Qt.RightEdge
    width: Math.min(parent.width, 460)
    height: parent.height
    modal: true
    padding: 20
    onOpened: composer.forceActiveFocus()
    background: Rectangle { color: brandIdentity.colors.surface }
    ColumnLayout {
        anchors.fill: parent
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
            text: "AI пока не подключён. Отправка сообщений станет доступна после подключения. Вы можете записать запрос здесь — он останется, пока приложение открыто."
            color: brandIdentity.colors.cyan
            Layout.fillWidth: true; wrapMode: Text.WordWrap
        }
        Item { Layout.fillHeight: true }
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(180, panel.height * 0.3)
            contentWidth: availableWidth
            TextArea {
                id: composer
                objectName: "chatComposer"
                placeholderText: "Какое приложение вам нужно?"
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                Accessible.name: "Сообщение AI"
            }
        }
        Button {
            objectName: "sendChatMessage"
            text: "Отправить"
            enabled: false
            Layout.alignment: Qt.AlignRight
        }
    }
}
