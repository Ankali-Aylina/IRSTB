import QtQuick
import QtQuick.Controls

Rectangle {
    id: titleBar
    height: 44
    color: AppTheme.bgPanel
    radius: 8

    signal closeRequested()

    MouseArea {
        anchors.fill: parent
        onPressed: Window.window.startSystemMove()
    }

    ThemeIcon {
        id: appLogo
        source: "qrc:/TCV3/res/icon/TreeNetWork_128.png"
        iconWidth: 26
        iconHeight: 26
        color: AppTheme.accent
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
    }

    Text {
        id: titleText
        anchors.left: appLogo.right
        anchors.leftMargin: 10
        anchors.right: windowButtons.left
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: qsTr("智能散热小桌板")
        color: AppTheme.textPrimary
        font.pixelSize: 15
        font.family: "Microsoft YaHei"
        font.bold: true
        elide: Text.ElideRight
        clip: true
    }

    Row {
        id: windowButtons
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        WindowButton {
            source: "qrc:/TCV3/res/icon/minimize.png"
            onClicked: Window.window.hide()
        }

        WindowButton {
            source: Window.window.visibility === Window.Maximized
                    ? "qrc:/TCV3/res/icon/normal.png"
                    : "qrc:/TCV3/res/icon/maximize.png"
            onClicked: {
                if (Window.window.visibility === Window.Maximized)
                    Window.window.showNormal()
                else
                    Window.window.showMaximized()
            }
        }

        WindowButton {
            source: "qrc:/TCV3/res/icon/close.png"
            hoverColor: AppTheme.dangerHover
            onClicked: titleBar.closeRequested()
        }
    }
}