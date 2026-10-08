import QtQuick
import QtQuick.Controls

Item {
    id: homePage
    property int fanMode: 0
    signal fanModeRequested(int mode)

    Text {
        id: homeTitle
        text: qsTr("实时温度")
        color: AppTheme.textSecondary
        font.pixelSize: 13
        anchors.top: parent.top
        anchors.left: parent.left
    }

    Row {
        id: tempCards
        anchors.top: homeTitle.bottom
        anchors.topMargin: 12
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 16

        TempCard {
            width: (parent.width - parent.spacing) / 2
            height: 150
            title: qsTr("CPU 温度")
            temperature: bridge.cpuTemp
            warningThreshold: bridge.warningCpu
            normalIcon: "qrc:/TCV3/res/icon/cpuTemp.png"
            // 注意：必须用白色源图，不能用 cpuTemp-red.png —— ThemeIcon 的
            // MultiEffect 着色是「colorizationColor × 源图亮度」，拿已经是红色的
            // 源图去着色会得到 #3e0e09 这种暗红褐色，而不是 AppTheme.danger。
            warningIcon: "qrc:/TCV3/res/icon/cpuTemp.png"
        }

        TempCard {
            width: (parent.width - parent.spacing) / 2
            height: 150
            title: qsTr("GPU 温度")
            temperature: bridge.gpuTemp
            warningThreshold: bridge.warningGpu
            normalIcon: "qrc:/TCV3/res/icon/gpuTemp.png"
            // 同上：必须用白色源图，着色由 AppTheme.danger 负责
            warningIcon: "qrc:/TCV3/res/icon/gpuTemp.png"
        }
    }

    Rectangle {
        id: blePanel
        anchors.top: tempCards.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.right: parent.right
        height: 84
        radius: 10
        color: AppTheme.bgPanel

        ThemeIcon {
            id: bleIcon
            iconWidth: 36
            iconHeight: 36
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            // BLE_On / BLE_Off / BLE_Error 三个文件其实是同一个蓝牙字形，只是颜色不同
            // （On=白，Off=猩红，Error=猩红+白）。ThemeIcon 的 MultiEffect 着色是
            // 「colorizationColor × 源图亮度」，不能拿已经带色的图去着色，否则会变暗：
            // 用 BLE_Off.png（#dc143c，亮度≈0.26）配 textSecondary 会得到 #202020 近黑，
            // 用 BLE_Error.png 配 danger 会得到 #621b14 暗褐。统一用白色的 BLE_On.png，
            // 颜色完全交给下面的 color 决定。
            source: "qrc:/TCV3/res/icon/BLE_On.png"
            // 状态 2 = 连接中 / 重连中（QmlBridge 对这两种情况都用 BleConnecting），显示天蓝色
            color: bridge.bleState === 3 ? AppTheme.success
                 : bridge.bleState === 4 ? AppTheme.danger
                 : bridge.bleState === 2 ? AppTheme.info
                 : AppTheme.textSecondary
        }

        Row {
            anchors.left: bleIcon.right
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 14

            Column {
                spacing: 4
                Text {
                    text: qsTr("蓝牙连接")
                    color: AppTheme.textSecondary
                    font.pixelSize: 12
                }
                Text {
                    text: bridge.bleStatusText
                    color: bridge.bleState === 3 ? AppTheme.success
                         : bridge.bleState === 4 ? AppTheme.danger
                         : bridge.bleState === 2 ? AppTheme.info
                         : AppTheme.textPrimary
                    font.pixelSize: 15
                }
            }

            Rectangle {
                id: busyTrack
                width: 110
                height: 4
                radius: 2
                color: AppTheme.bgTrack
                anchors.verticalCenter: parent.verticalCenter
                visible: bridge.bleState === 1 || bridge.bleState === 2

                Rectangle {
                    id: busyBar
                    width: 40
                    height: parent.height
                    radius: 2
                    color: AppTheme.accent
                    x: 0

                    SequentialAnimation on x {
                        running: busyTrack.visible
                        loops: Animation.Infinite
                        NumberAnimation { from: 0; to: busyTrack.width - busyBar.width; duration: 800 }
                        NumberAnimation { from: busyTrack.width - busyBar.width; to: 0; duration: 800 }
                    }
                }
            }
        }

        Button {
            focusPolicy: Qt.NoFocus
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("重连")
            onClicked: bridge.reconnectBle()

            background: Rectangle {
                radius: 6
                color: parent.hovered ? AppTheme.accentHover : AppTheme.accent
                implicitWidth: 64
                implicitHeight: 32
            }

            contentItem: Text {
                text: parent.text
                color: AppTheme.textOnAccent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Row {
        id: controlRow
        anchors.top: blePanel.bottom
        anchors.topMargin: 12
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 16

        Rectangle {
            width: (parent.width - parent.spacing) / 2
            height: 100
            radius: 10
            color: AppTheme.bgPanel

            Text {
                text: qsTr("风扇模式")
                color: AppTheme.textSecondary
                font.pixelSize: 13
                anchors.top: parent.top
                anchors.topMargin: 14
                anchors.left: parent.left
                anchors.leftMargin: 16
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 16
                spacing: 10

                FanModeButton {
                    text: qsTr("自动")
                    active: homePage.fanMode === 0
                    onClicked: homePage.fanModeRequested(0)
                }

                FanModeButton {
                    text: qsTr("静音")
                    active: homePage.fanMode === 1
                    onClicked: homePage.fanModeRequested(1)
                }

                FanModeButton {
                    text: qsTr("全速")
                    active: homePage.fanMode === 2
                    onClicked: homePage.fanModeRequested(2)
                }
            }
        }

        Rectangle {
            width: (parent.width - parent.spacing) / 2
            height: 100
            radius: 10
            color: AppTheme.bgPanel

            Text {
                text: qsTr("数据发送延时")
                color: AppTheme.textSecondary
                font.pixelSize: 13
                anchors.top: parent.top
                anchors.topMargin: 14
                anchors.left: parent.left
                anchors.leftMargin: 16
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 16
                spacing: 10

                IconButton {
                    source: "qrc:/TCV3/res/icon/minus.png"
                    onClicked: bridge.changeDelay(-1)
                }

                Rectangle {
                    width: 72
                    height: 34
                    radius: 6
                    color: AppTheme.bgTrack
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        anchors.centerIn: parent
                        text: bridge.delayUiSeconds + " s"
                        color: AppTheme.textPrimary
                        font.pixelSize: 14
                    }
                }

                IconButton {
                    source: "qrc:/TCV3/res/icon/add.png"
                    onClicked: bridge.changeDelay(1)
                }

                Button {
                    focusPolicy: Qt.NoFocus
                    text: qsTr("应用")
                    onClicked: bridge.applyDelay()

                    background: Rectangle {
                        radius: 6
                        color: parent.hovered ? AppTheme.accentHover : AppTheme.accent
                        implicitWidth: 64
                        implicitHeight: 32
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

    Rectangle {
        id: shutdownPanel
        anchors.top: controlRow.bottom
        anchors.topMargin: 12
        anchors.left: parent.left
        anchors.right: parent.right
        height: 88
        radius: 10
        color: AppTheme.bgPanel

        Text {
            id: shutdownLabel
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 16
            text: qsTr("关机散热")
            color: AppTheme.textPrimary
            font.pixelSize: 13
        }

        Text {
            id: shutdownHint
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: shutdownControls.left
            anchors.rightMargin: 16
            anchors.top: shutdownLabel.bottom
            anchors.topMargin: 6
            text: qsTr("风扇满速运行设定时长后自动断电；倒计时由下位机独立完成，期间蓝牙断开或电脑关机均不受影响")
            color: AppTheme.textSecondary
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }

        Row {
            id: shutdownControls
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            IconButton {
                source: "qrc:/TCV3/res/icon/minus.png"
                onClicked: bridge.changeShutdownMinutes(-1)
            }

            Rectangle {
                width: 72
                height: 34
                radius: 6
                color: AppTheme.bgTrack
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: bridge.shutdownMinutes + " min"
                    color: AppTheme.textPrimary
                    font.pixelSize: 14
                }
            }

            IconButton {
                source: "qrc:/TCV3/res/icon/add.png"
                onClicked: bridge.changeShutdownMinutes(1)
            }

            Button {
                focusPolicy: Qt.NoFocus
                text: qsTr("开始散热")
                onClicked: bridge.startShutdownCooling()

                background: Rectangle {
                    radius: 6
                    color: parent.pressed ? AppTheme.dangerHover
                         : parent.hovered ? AppTheme.dangerHover
                         : AppTheme.danger
                    implicitWidth: 84
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

    Rectangle {
        id: shutdownNoticeBar
        anchors.top: shutdownPanel.bottom
        anchors.topMargin: 12
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        radius: 8
        color: AppTheme.dangerSoft
        border.color: AppTheme.danger
        border.width: 1
        visible: bridge.shutdownNotice.length > 0

        ThemeIcon {
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/TCV3/res/icon/warning_line.png"
            iconWidth: 16
            iconHeight: 16
            color: AppTheme.danger
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 40
            anchors.right: noticeClose.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: bridge.shutdownNotice
            color: AppTheme.textPrimary
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        Text {
            id: noticeClose
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            text: "\u2715"
            color: AppTheme.danger
            font.pixelSize: 13

            MouseArea {
                anchors.fill: parent
                anchors.margins: -6
                cursorShape: Qt.PointingHandCursor
                onClicked: bridge.dismissShutdownNotice()
            }
        }
    }
}