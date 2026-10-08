import QtQuick
import QtQuick.Controls

Dialog {
    id: eggDialog
    modal: true
    width: 420
    x: (Overlay.overlay.width - width) / 2
    y: (Overlay.overlay.height - height) / 2
    standardButtons: Dialog.NoButton
    padding: 20
    background: Rectangle { color: AppTheme.bgDialog; radius: 10; border.color: AppTheme.border }

    header: Rectangle {
        color: AppTheme.bgPanel
        radius: 10
        height: 46

        Label {
            anchors.centerIn: parent
            text: qsTr("彩蛋")
            color: AppTheme.textPrimary
        }
    }

    contentItem: Column {
        spacing: 14
        width: parent.width

        ThemeIcon {
            source: "qrc:/TCV3/res/icon/TreeNetWork_128.png"
            iconWidth: 56
            iconHeight: 56
            color: AppTheme.accent
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Text {
            width: parent.width
            text: qsTr("啊~真的Rich!")
            color: AppTheme.accent
            font.pixelSize: 24
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Text {
            width: parent.width
            text: qsTr("感谢所有支持这个项目的朋友！")
            color: AppTheme.textTertiary
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            lineHeight: 1.4
        }
    }

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
            onClicked: eggDialog.close()

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
}