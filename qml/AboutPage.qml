import QtQuick
import QtQuick.Controls

Item {
    id: aboutPage
    signal showLogDialog()
    signal showEggDialog()

    Text {
        text: qsTr("关于")
        color: AppTheme.textSecondary
        font.family: AppTheme.fontBody
        font.pixelSize: 13
    }

    Rectangle {
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.left: parent.left
        anchors.right: parent.right
        height: 142
        radius: AppTheme.radiusPanel
        border.width: AppTheme.nasa ? 1 : 0
        border.color: AppTheme.border
        color: AppTheme.bgPanel

        Item {
            width: 64
            height: 64
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter

            ThemeIcon {
                anchors.fill: parent
                source: "qrc:/TCV3/res/icon/TreeNetWork_128.png"
                iconWidth: 64
                iconHeight: 64
                color: AppTheme.accent
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: qsTr("点击有惊喜 🥚")
                onClicked: aboutPage.showEggDialog()
            }
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 104
            anchors.top: parent.top
            anchors.topMargin: 20
            text: qsTr("智能散热小桌板")
            color: AppTheme.textPrimary
            font.family: AppTheme.fontBody
            font.pixelSize: 17
            font.bold: true
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 104
            anchors.top: parent.top
            anchors.topMargin: 50
            text: qsTr("版本 V") + bridge.appVersion
            color: AppTheme.textSecondary
            font.family: AppTheme.fontBody
            font.pixelSize: 12
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 104
            anchors.top: parent.top
            anchors.topMargin: 76
            text: qsTr("根据电脑温度自动调节风扇转速")
            color: AppTheme.textTertiary
            font.family: AppTheme.fontBody
            font.pixelSize: 11
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 104
            anchors.top: parent.top
            anchors.topMargin: 96
            font.family: AppTheme.fontBody
            font.pixelSize: 11
            color: bridge.firmwareSupportsShutdown ? AppTheme.textTertiary : AppTheme.danger
            text: {
                if (bridge.firmwareVersion.length > 0) {
                    var t = qsTr("下位机固件 v") + bridge.firmwareVersion
                    if (bridge.firmwareProtocol.length > 0)
                        t += "（协议 " + bridge.firmwareProtocol + "）"
                    if (!bridge.firmwareSupportsShutdown)
                        t += qsTr("　版本过旧，关机散热不可用")
                    return t
                }
                return bridge.bleState === 3
                       ? qsTr("下位机固件版本未知（固件不支持版本查询）")
                       : qsTr("下位机固件版本：未连接")
            }
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 104
            anchors.top: parent.top
            anchors.topMargin: 116
            text: qsTr("感谢 cyjycx 的资金支持！")
            color: AppTheme.accent
            font.family: AppTheme.fontBody
            font.pixelSize: 12
            font.bold: true
        }
    }

    Row {
        anchors.top: parent.top
        anchors.topMargin: 188
        spacing: 14

        ActionButton {
            text: qsTr("更新日志")
            iconSource: "qrc:/TCV3/res/icon/updataLog.png"
            onClicked: aboutPage.showLogDialog()
        }

        ActionButton {
            text: qsTr("支持与文档")
            iconSource: "qrc:/TCV3/res/icon/github-line.png"
            onClicked: bridge.openSupportUrl()
        }

        ActionButton {
            text: bridge.otaRunning ? qsTr("升级中…") : qsTr("升级固件")
            iconSource: "qrc:/TCV3/res/icon/warning_line.png"
            enabled: !bridge.otaRunning
            opacity: enabled ? 1.0 : 0.5
            onClicked: bridge.chooseFirmwareAndUpgrade()
        }

        ActionButton {
            id: checkUpdateButton
            text: bridge.updateState === 1 ? qsTr("检查中…") : qsTr("检查更新")
            iconComponent: downloadIconComponent
            enabled: !bridge.otaRunning && bridge.updateState !== 1
            opacity: enabled ? 1.0 : 0.5
            onClicked: bridge.checkForUpdates()
        }
    }

    // 「检查更新」的矢量图标（与 ThemeIcon 的位图二选一，由 ActionButton.iconComponent 切换）
    Component {
        id: downloadIconComponent
        IconDownload {
            size: 18
            color: AppTheme.textSecondary
        }
    }

    // 版本检测结果面板。与 otaPanel 共用同一位置：
    // OTA 进行中时让位给 OTA，其余时间显示检查结果（两者互斥，不会叠在一起）。
    Rectangle {
        id: updatePanel
        anchors.top: parent.top
        anchors.topMargin: 244
        anchors.left: parent.left
        anchors.right: parent.right
        height: 72
        radius: AppTheme.radiusPanel
        border.width: AppTheme.nasa ? 1 : 0
        border.color: AppTheme.border
        color: AppTheme.bgPanel
        visible: !bridge.otaRunning && bridge.updateState >= 2

        Rectangle {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.verticalCenter: parent.verticalCenter
            width: 3
            height: 26
            color: bridge.updateState === 3 ? AppTheme.accent
                 : bridge.updateState === 4 ? AppTheme.danger
                 : AppTheme.success
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 33
            anchors.right: parent.right
            anchors.rightMargin: 158
            anchors.verticalCenter: parent.verticalCenter
            text: bridge.updateText
            color: bridge.updateState === 3 ? AppTheme.accent
                 : bridge.updateState === 4 ? AppTheme.danger
                 : AppTheme.textSecondary
            font.family: AppTheme.fontBody
            font.pixelSize: 12
            elide: Text.ElideRight
            wrapMode: Text.WordWrap
            maximumLineCount: 2
        }

        ActionButton {
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            visible: bridge.updateState === 3
            implicitWidth: 132
            implicitHeight: 36
            text: qsTr("前往下载")
            iconSource: "qrc:/TCV3/res/icon/github-line.png"
            onClicked: bridge.openUpdateUrl()
        }
    }

    Rectangle {
        id: otaPanel
        anchors.top: parent.top
        anchors.topMargin: 244
        anchors.left: parent.left
        anchors.right: parent.right
        height: 72
        radius: AppTheme.radiusPanel
        border.width: AppTheme.nasa ? 1 : 0
        border.color: AppTheme.border
        color: AppTheme.bgPanel
        visible: bridge.otaRunning
             || (bridge.otaStatusText.length > 0 && bridge.otaPercent > 0)

        Text {
            id: otaTitle
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 12
            text: qsTr("固件升级")
            color: AppTheme.textPrimary
            font.family: AppTheme.fontBody
            font.pixelSize: 13
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 12
            text: bridge.otaPercent + "%"
            color: bridge.otaPercent === 100 ? AppTheme.accent : AppTheme.textSecondary
            font.family: AppTheme.fontBody
            font.pixelSize: 13
            font.bold: true
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: otaTitle.bottom
            anchors.topMargin: 6
            text: bridge.otaStatusText
            color: bridge.otaStatusText.indexOf(qsTr("失败")) >= 0
                   ? AppTheme.danger : AppTheme.textTertiary
            font.family: AppTheme.fontBody
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        Rectangle {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            height: 6
            radius: AppTheme.radiusTick
            color: AppTheme.border

            Rectangle {
                width: parent.width * Math.max(0, Math.min(100, bridge.otaPercent)) / 100
                height: parent.height
                radius: AppTheme.radiusTick
                color: AppTheme.accent
                Behavior on width { NumberAnimation { duration: 120 } }
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.rightMargin: 62
            anchors.top: parent.top
            anchors.topMargin: 8
            width: 52
            height: 20
            visible: bridge.otaRunning
            cursorShape: Qt.PointingHandCursor

            Text {
                anchors.centerIn: parent
                text: qsTr("取消")
                color: AppTheme.danger
                font.family: AppTheme.fontBody
                font.pixelSize: 11
            }

            onClicked: bridge.cancelFirmwareUpgrade()
        }
    }
}