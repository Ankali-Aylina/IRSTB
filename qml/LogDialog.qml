import QtQuick
import QtQuick.Controls

Dialog {
    id: logDialog
    modal: true
    width: 580
    height: 440
    x: (Overlay.overlay.width - width) / 2
    y: (Overlay.overlay.height - height) / 2
    standardButtons: Dialog.NoButton

    footer: Rectangle {
        height: 48
        color: "transparent"

        Button {
            text: qsTr("关闭")
            focusPolicy: Qt.NoFocus
            implicitWidth: 80
            implicitHeight: 30
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            onClicked: logDialog.close()

            background: Rectangle {
                radius: 6
                color: parent.hovered ? AppTheme.accentHover : AppTheme.accent
                implicitWidth: 80
                implicitHeight: 30
            }

            contentItem: Text {
                text: parent.text
                color: AppTheme.textOnAccent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    background: Rectangle {
        color: AppTheme.bgDialog
        radius: 10
        border.color: AppTheme.border
    }

    header: Rectangle {
        color: AppTheme.bgPanel
        radius: 10
        height: 46

        Label {
            anchors.centerIn: parent
            text: qsTr("更新日志")
            color: AppTheme.textPrimary
        }
    }

    contentItem: ScrollView {
        TextArea {
            readOnly: true
            text: bridge.readUpdateLog()
            // 更新日志是 Markdown 原文（res/updatalog.md 直接读出来），
            // TextArea 默认按 PlainText 渲染，所以标题/列表/粗体都会连符号一起显示。
            // 用 MarkdownText 让 Qt 自己渲染（支持标题、列表、引用、行内代码、粗体）。
            textFormat: TextEdit.MarkdownText
            font.family: "Microsoft YaHei"
            font.pixelSize: 12
            background: Rectangle { color: AppTheme.bgPanel; radius: 6 }
            wrapMode: TextEdit.Wrap
        }
    }
}