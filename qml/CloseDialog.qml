import QtQuick
import QtQuick.Controls

Dialog {
    id: closeDialog
    modal: true
    width: 420
    x: (Overlay.overlay.width - width) / 2
    y: (Overlay.overlay.height - height) / 2
    standardButtons: Dialog.NoButton
    background: Rectangle { color: AppTheme.bgDialog; radius: 10; border.color: AppTheme.border }

    header: Rectangle {
        color: AppTheme.bgPanel
        radius: 10
        height: 50

        ThemeIcon {
            id: closeDlgLogo
            source: "qrc:/TCV3/res/icon/TreeNetWork_128.png"
            iconWidth: 22
            iconHeight: 22
            color: AppTheme.accent
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
        }

        Label {
            anchors.left: closeDlgLogo.right
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("退出提示")
            color: AppTheme.textPrimary
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: Column {
        spacing: 12

        Text {
            text: qsTr("要退出程序，还是最小化到系统托盘？")
            color: AppTheme.textPrimary
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }

        Text {
            text: qsTr("最小化到托盘后，程序仍会在后台监控温度，双击托盘图标可恢复窗口。")
            color: AppTheme.textTertiary
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            lineHeight: 1.4
        }

        Item {
            width: parent.width
            height: 34

            Button {
                focusPolicy: Qt.NoFocus
                text: qsTr("退出程序")
                implicitWidth: 100
                implicitHeight: 34
                anchors.left: parent.left
                anchors.leftMargin: 2
                onClicked: {
                    closeDialog.close()
                    bridge.quitApp()
                }

                background: Rectangle {
                    radius: 6
                    color: "transparent"
                    border.color: parent.hovered ? AppTheme.danger : AppTheme.border
                    border.width: 1
                    implicitWidth: 100
                    implicitHeight: 34
                }

                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? AppTheme.danger : AppTheme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            Button {
                focusPolicy: Qt.NoFocus
                text: qsTr("最小化到托盘")
                implicitWidth: 140
                implicitHeight: 34
                anchors.right: parent.right
                anchors.rightMargin: 2
                onClicked: {
                    closeDialog.close()
                    Window.window.hide()
                }

                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? AppTheme.accentHover : AppTheme.accent
                    implicitWidth: 140
                    implicitHeight: 34
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
}