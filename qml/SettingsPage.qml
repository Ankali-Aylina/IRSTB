import QtQuick
import QtQuick.Controls

Item {
    id: settingsPage

    Text {
        text: qsTr("设置")
        color: AppTheme.textSecondary
        font.pixelSize: 13
    }

    Rectangle {
        id: themePanel
        // 原来这里上方还有一个同样字号/颜色的「外观」分组标题，两层平级标签叠在
        // 一起没有层级、且「设置」又与导航栏重复，已删掉。
        // 现在与「关于」页保持同一节奏：标题 + 30px + 首个面板。
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.left: parent.left
        anchors.right: parent.right
        height: 76
        radius: 10
        color: AppTheme.bgPanel

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 14
            text: qsTr("主题模式")
            color: AppTheme.textPrimary
            font.pixelSize: 13
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14
            text: qsTr("跟随系统或手动切换浅色 / 深色")
            color: AppTheme.textTertiary
            font.pixelSize: 11
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            ThemeButton { text: qsTr("跟随系统"); active: bridge.themeMode === 0; onClicked: bridge.setThemeMode(0) }
            ThemeButton { text: qsTr("浅色"); active: bridge.themeMode === 1; onClicked: bridge.setThemeMode(1) }
            ThemeButton { text: qsTr("深色"); active: bridge.themeMode === 2; onClicked: bridge.setThemeMode(2) }
        }
    }

    Rectangle {
        id: backdropPanel
        anchors.top: themePanel.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.right: parent.right
        height: 76
        radius: 10
        color: AppTheme.bgPanel

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 14
            text: qsTr("背景材质")
            color: AppTheme.textPrimary
            font.pixelSize: 13
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14
            text: qsTr("Windows 11 云母 (Mica) / 亚克力 (Acrylic)，旧系统自动降级")
            color: AppTheme.textTertiary
            font.pixelSize: 11
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            ThemeButton { text: qsTr("云母"); active: bridge.backdropType === 1; onClicked: bridge.setBackdropType(1) }
            ThemeButton { text: qsTr("亚克力"); active: bridge.backdropType === 2; onClicked: bridge.setBackdropType(2) }
            ThemeButton { text: qsTr("无"); active: bridge.backdropType === 0; onClicked: bridge.setBackdropType(0) }
        }
    }

    Rectangle {
        id: autostartPanel
        anchors.top: backdropPanel.bottom
        anchors.topMargin: 14
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        radius: 10
        color: AppTheme.bgPanel

        ThemeIcon {
            id: asIcon
            source: "qrc:/TCV3/res/icon/autoStart.png"
            iconWidth: 28
            iconHeight: 28
            color: AppTheme.textSecondary
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            anchors.left: asIcon.right
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("开机自启动")
            color: AppTheme.textPrimary
            font.pixelSize: 14
        }

        ToggleSwitch {
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            checked: bridge.autoStartEnabled
            onClicked: bridge.toggleAutoStart()
        }
    }

    Rectangle {
        id: notifyPanel
        anchors.top: autostartPanel.bottom
        anchors.topMargin: 14
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        radius: 10
        color: AppTheme.bgPanel

        ThemeIcon {
            id: notifyIcon
            source: "qrc:/TCV3/res/icon/warning_line.png"
            iconWidth: 28
            iconHeight: 28
            color: AppTheme.textSecondary
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            anchors.left: notifyIcon.right
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 15
            text: qsTr("系统通知")
            color: AppTheme.textPrimary
            font.pixelSize: 14
        }

        Text {
            anchors.left: notifyIcon.right
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 36
            text: qsTr("关机散热结束或指令下发失败时，在右下角弹出提示（窗口最小化也可见）")
            color: AppTheme.textTertiary
            font.pixelSize: 11
            elide: Text.ElideRight
            anchors.right: notifyToggle.left
            anchors.rightMargin: 10
        }

        ToggleSwitch {
            id: notifyToggle
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            checked: bridge.notifyEnabled
            onClicked: bridge.toggleNotify()
        }
    }

    Rectangle {
        anchors.top: notifyPanel.bottom
        anchors.topMargin: 14
        anchors.left: parent.left
        anchors.right: parent.right
        height: 64
        radius: 10
        color: AppTheme.bgPanel

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("重置设置")
            color: AppTheme.textPrimary
            font.pixelSize: 14
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 38
            text: qsTr("删除配置文件并重启应用")
            color: AppTheme.textTertiary
            font.pixelSize: 11
        }

        Button {
            focusPolicy: Qt.NoFocus
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("重置")
            onClicked: bridge.resetSettings()

            background: Rectangle {
                radius: 6
                color: parent.hovered ? AppTheme.dangerHover : AppTheme.danger
                implicitWidth: 60
                implicitHeight: 28
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