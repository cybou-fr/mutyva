import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: dialog
    property bool welcomeMode: false
    objectName: welcomeMode ? "welcomeDialog" : "aboutDialog"
    title: welcomeMode ? "Добро пожаловать в MUTYVA" : "О MUTYVA"
    anchors.centerIn: parent
    width: Math.min(parent.width - 32, 620)
    height: Math.min(parent.height - 32, identityContent.implicitHeight + identityHeader.implicitHeight + identityFooter.implicitHeight + topPadding + bottomPadding)
    modal: true
    closePolicy: Popup.CloseOnEscape
    padding: 20
    background: Rectangle {
        radius: 22; color: "#0B1125"
        border.width: 1; border.color: brandIdentity.colors.violet
    }
    header: Label {
        id: identityHeader
        text: dialog.title; textFormat: Text.PlainText
        padding: 20; color: brandIdentity.colors.foreground
        font.pixelSize: 20; font.bold: true; wrapMode: Text.WordWrap
    }
    contentItem: ScrollView {
        clip: true; contentWidth: availableWidth
        ColumnLayout {
            id: identityContent
            width: parent.width
            spacing: 16
            Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: Math.min(230, width * 0.57)
                radius: 16; color: "#00030A"
                Image {
                    objectName: "identityLogo"
                    anchors.fill: parent; anchors.margins: 12
                    source: brandIdentity.logoUrl; fillMode: Image.PreserveAspectFit
                    mipmap: true
                }
            }
            Label {
                text: "Одно приложение. Любая форма."
                color: brandIdentity.colors.foreground; font.pixelSize: 23; font.bold: true
                Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Label {
                text: "Собирайте свой инструмент из компонентов, проверяйте изменения в предпросмотре и сохраняйте поколения интерфейса."
                font.pixelSize: 14
                color: brandIdentity.colors.muted; Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Label {
                text: "Локальный runtime · 12 компонентов · история конфигурации\nАвторизация ChatGPT и AI-дизайнер ещё в разработке."
                font.pixelSize: 13
                color: brandIdentity.colors.cyan; Layout.fillWidth: true; wrapMode: Text.WordWrap
            }
            Label {
                text: "MUTYVA · " + brandIdentity.version
                color: brandIdentity.colors.muted; Layout.fillWidth: true
            }
        }
    }
    footer: DialogButtonBox {
        id: identityFooter
        Button {
            objectName: dialog.welcomeMode ? "startMutyva" : "closeAbout"
            text: dialog.welcomeMode ? "Начать" : "Закрыть"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        onAccepted: {
            if (dialog.welcomeMode) brandIdentity.finishWelcome()
            dialog.close()
        }
    }
}
