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
        font.family: AppTheme.fontBody
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
            iconKind: "cpu"
        }

        TempCard {
            width: (parent.width - parent.spacing) / 2
            height: 150
            title: qsTr("GPU 温度")
            temperature: bridge.gpuTemp
            warningThreshold: bridge.warningGpu
            iconKind: "gpu"
        }
    }

    Rectangle {
        id: blePanel
        anchors.top: tempCards.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.right: parent.right
        height: 84
        radius: AppTheme.radiusPanel
        border.width: AppTheme.nasa ? 1 : 0
        border.color: AppTheme.border
        color: AppTheme.bgPanel

        // NASA 朋克：四角铆钉（仅 NASA 模式可见）
        NasaRivets { anchors.fill: parent }

        // 蓝牙图标：QML 矢量绘制（替代 BLE_On/Off/Error 三个 PNG）。
        // 原来那三个文件是同一字形、三种颜色，还要靠 ThemeIcon 的 MultiEffect 着色，
        // 一旦拿带色的源图去着色就会变暗（断开被压成近黑、失败压成暗褐）。
        // 现在只有一个字形，颜色直接由状态决定。
        IconBluetooth {
            id: bleIcon
            size: 36
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
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
                    font.family: AppTheme.fontBody
                    font.pixelSize: 12
                }
                Text {
                    text: bridge.bleStatusText
                    color: bridge.bleState === 3 ? AppTheme.success
                         : bridge.bleState === 4 ? AppTheme.danger
                         : bridge.bleState === 2 ? AppTheme.info
                         : AppTheme.textPrimary
                    font.pixelSize: 15
                    font.family: AppTheme.nasa ? AppTheme.fontMono : AppTheme.fontBody
                    font.letterSpacing: AppTheme.nasa ? 1 : 0
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
                radius: AppTheme.radiusButton
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
            radius: AppTheme.radiusPanel
            border.width: AppTheme.nasa ? 1 : 0
            border.color: AppTheme.border
            color: AppTheme.bgPanel

            Text {
                text: qsTr("风扇模式")
                color: AppTheme.textSecondary
                font.family: AppTheme.fontBody
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
            radius: AppTheme.radiusPanel
            border.width: AppTheme.nasa ? 1 : 0
            border.color: AppTheme.border
            color: AppTheme.bgPanel

            Text {
                text: qsTr("数据发送延时")
                color: AppTheme.textSecondary
                font.family: AppTheme.fontBody
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
                    radius: AppTheme.radiusButton
                    color: AppTheme.bgTrack
                    anchors.verticalCenter: parent.verticalCenter

                    Text {
                        anchors.centerIn: parent
                        text: bridge.delayUiSeconds + " s"
                        color: AppTheme.textPrimary
                        font.pixelSize: 14
                        // NASA 朋克：数值用等宽字体
                        font.family: AppTheme.nasa ? AppTheme.fontMono : AppTheme.fontBody
                        font.letterSpacing: AppTheme.nasa ? 1 : 0
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
                        radius: AppTheme.radiusButton
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
        radius: AppTheme.radiusPanel
        border.width: AppTheme.nasa ? 1 : 0
        border.color: AppTheme.border
        color: AppTheme.bgPanel

        Text {
            id: shutdownLabel
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 16
            text: qsTr("关机散热")
            color: AppTheme.textPrimary
            font.family: AppTheme.fontBody
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
            font.family: AppTheme.fontBody
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
                radius: AppTheme.radiusButton
                color: AppTheme.bgTrack
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    anchors.centerIn: parent
                    text: bridge.shutdownMinutes + " min"
                    color: AppTheme.textPrimary
                    font.pixelSize: 14
                    // NASA 朋克：数值用等宽字体
                    font.family: AppTheme.nasa ? AppTheme.fontMono : AppTheme.fontBody
                    font.letterSpacing: AppTheme.nasa ? 1 : 0
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
                    radius: AppTheme.radiusButton
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
        radius: AppTheme.radiusSurface
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
            font.family: AppTheme.fontBody
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
            font.family: AppTheme.fontBody
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