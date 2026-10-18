import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 10
    Label { text: "Замысел и контекст AI"; color: "white"; font.bold: true; font.pixelSize: 18 }
    Label {
        text: "AI получает цель, установленную палитру, текущий интерфейс, допустимые операции и ваш запрос. Ответ становится предпросмотром, а не выполняется напрямую."
        color: "#a3b0c8"; Layout.fillWidth: true; wrapMode: Text.WordWrap
    }
    ScrollView {
        Layout.fillWidth: true; Layout.preferredHeight: 90
        contentWidth: availableWidth
        TextArea {
            id: designRequest
            objectName: "designRequest"
            text: "Добавь карточку влажности после прогноза"
            placeholderText: "Как должно измениться приложение?"
            wrapMode: TextEdit.Wrap
            Accessible.name: "Запрос на изменение приложения"
        }
    }
    Button {
        objectName: "prepareDesignerContext"
        text: "Подготовить контекст"
        enabled: !appDocument.hasPreview
        onClicked: appDocument.prepareDesignerContext(designRequest.text)
    }
    Label {
        text: appDocument.designerMessage
        textFormat: Text.PlainText
        visible: text.length > 0
        color: "#9befdc"; Layout.fillWidth: true; wrapMode: Text.WordWrap
    }
    Label {
        text: "Сейчас работает локальная подготовка. Подключение ChatGPT — следующий этап. Показания погоды, история и учётные данные в пакет не включаются; подписи виджетов включаются."
        color: "#a3b0c8"; Layout.fillWidth: true; wrapMode: Text.WordWrap
    }
    CheckBox { id: inspectContext; text: "Посмотреть полный пакет контекста" }
    ScrollView {
        visible: inspectContext.checked
        Layout.fillWidth: true; Layout.preferredHeight: 260
        contentWidth: availableWidth
        TextArea {
            objectName: "designerContextOutput"
            text: appDocument.designerContextJson
            readOnly: true; selectByMouse: true
            wrapMode: TextEdit.WrapAnywhere; font.family: "monospace"
            Accessible.name: "Подготовленный контекст AI"
        }
    }
}
