import QtQuick
import QtQuick.Controls

Item {
    id: aboutPage
    signal showLogDialog()
    signal showEggDialog()

    Text {
        text: qsTr("关于")
        color: AppTheme.textSecondary
        font.pixelSize: 13
    }

    Rectangle {
        anchors.top: parent.top
        anchors.topMargin: 30
        anchors.left: parent.left
        anchors.right: parent.right
        height: 142
        radius: 10
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
            font.pixelSize: 12
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 104
            anchors.top: parent.top
            anchors.topMargin: 76
            text: qsTr("根据电脑温度自动调节风扇转速")
            color: AppTheme.textTertiary
            font.pixelSize: 11
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 104
            anchors.top: parent.top
            anchors.topMargin: 96
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
    }

    Rectangle {
        id: otaPanel
        anchors.top: parent.top
        anchors.topMargin: 244
        anchors.left: parent.left
        anchors.right: parent.right
        height: 72
        radius: 10
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
            font.pixelSize: 13
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 12
            text: bridge.otaPercent + "%"
            color: bridge.otaPercent === 100 ? AppTheme.accent : AppTheme.textSecondary
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
            radius: 3
            color: AppTheme.border

            Rectangle {
                width: parent.width * Math.max(0, Math.min(100, bridge.otaPercent)) / 100
                height: parent.height
                radius: 3
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
                font.pixelSize: 11
            }

            onClicked: bridge.cancelFirmwareUpgrade()
        }
    }
}