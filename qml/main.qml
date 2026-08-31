import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Window

Window {
    id: root
    width: 1000
    height: 660
    minimumWidth: 880
    minimumHeight: 580
    visible: true
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "transparent"
    title: qsTr("智能散热小桌板")

    // ===== 状态 =====
    property int currentPage: 0      // 0=首页 1=设置 2=关于
    property int switchTarget: 0
    property bool switching: false
    property int fanMode: 0          // 0=自动 1=静音 2=全速

    // ===== 主题（WinUI 风格：Mica/Acrylic 材质 + 浅色/深色双模式） =====
    // systemDark 来自 bridge（读注册表 AppsUseLightTheme）——Qt.styleHints.colorScheme
    // 在部分系统上返回 Unknown/Light 导致"深色系统下渲染浅色主题"，不可靠
    property bool systemDark: bridge.systemDark
    property bool isDark: bridge.themeMode === 1 ? false : bridge.themeMode === 2 ? true : systemDark
    property var theme: isDark ? darkTheme : lightTheme

    onIsDarkChanged: bridge.applyDarkMode(isDark)
    Component.onCompleted: {
        // ===== 调试：验证主题状态（验证后可删除） =====
        console.log("isDark:", isDark)
        console.log("bridge.themeMode:", bridge.themeMode)
        console.log("bridge.systemDark:", bridge.systemDark)
        console.log("theme.accentSoft:", theme.accentSoft.toString())
        // ===== 调试结束 =====
        bridge.applyDarkMode(isDark)
    }

    // 浅色主题（WinUI Light 色板）
    QtObject {
        id: lightTheme
        readonly property color bgBase: "#E6F3F3F3"       // Mica 基底（浅）
        readonly property color bgPanel: "#B3FFFFFF"      // 面板/标题栏
        readonly property color bgTrack: "#E5E5E5"        // 进度轨道/输入底
        readonly property color bgHover: "#D9E5E5E5"      // 悬停底色
        readonly property color bgDialog: "#F2FFFFFF"     // 对话框
        readonly property color border: "#1F000000"       // 12% 黑边框
        readonly property color textPrimary: "#1B1B1B"
        readonly property color textSecondary: "#616161"
        readonly property color textTertiary: "#8A8A8A"
        readonly property color accent: "#0067C0"
        readonly property color accentHover: "#005BB8"
        readonly property color accentSoft: "#E6F0F9"   // 激活/悬停底色（不透明，10% accent 叠加白）
        readonly property color textOnAccent: "#FFFFFF"
        readonly property color danger: "#C42B1C"
        readonly property color dangerHover: "#A82417"
        readonly property color success: "#0F7B0F"
    }

    // 深色主题（WinUI Dark 色板）
    QtObject {
        id: darkTheme
        readonly property color bgBase: "#E6202020"       // Mica 基底（深）
        readonly property color bgPanel: "#B32B2B2B"
        readonly property color bgTrack: "#3D3D3D"
        readonly property color bgHover: "#4A4A4A"
        readonly property color bgDialog: "#F22B2B2B"
        readonly property color border: "#1FFFFFFF"       // 12% 白边框
        readonly property color textPrimary: "#FFFFFF"
        readonly property color textSecondary: "#A8A8A8" 
        readonly property color textTertiary: "#8A8A8A"
        readonly property color accent: "#4CC2FF"
        readonly property color accentHover: "#5FC9FF"
        readonly property color accentSoft: "#4A6A7A"  // 激活/悬停底色（不透明，20% accent 叠加面板）
        readonly property color textOnAccent: "#000000"
        readonly property color danger: "#FF99A4"
        readonly property color dangerHover: "#FFB3BB"
        readonly property color success: "#6CCB5F"
    }

    // Alt+F4 / 系统关闭 → 弹退出提示
    onClosing: (close) => {
        close.accepted = false
        closeDialog.open()
    }

    // 托盘"恢复" → 显示窗口
    Connections {
        target: bridge
        function onRestoreRequested() { root.show(); root.raise(); }
    }

    // ==================================================================
    // 根面板（圆角深色）
    // ==================================================================
    Rectangle {
        id: rootPanel
        anchors.fill: parent
        // 圆角必须与 DWM 系统窗口圆角（Win11 默认约 8px）一致，
        // 否则面板圆角外会露出材质"留空"带
        radius: 8
        color: theme.bgBase
        border.color: theme.border
        border.width: 1
        clip: true

        // ---------- 标题栏 ----------
        Rectangle {
            id: titleBar
            width: parent.width
            height: 44
            color: theme.bgPanel
            radius: 8

            MouseArea {
                anchors.fill: parent
                onPressed: root.startSystemMove()
            }

            ThemeIcon {
                id: appLogo
                source: "qrc:/TCV3/res/icon/TreeNetWork_128.png"
                iconWidth: 26; iconHeight: 26
                color: theme.accent
                anchors.left: parent.left; anchors.leftMargin: 14
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                id: titleText
                anchors.left: appLogo.right; anchors.leftMargin: 10
                anchors.right: windowButtons.left; anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("智能散热小桌板")
                color: theme.textPrimary
                font.pixelSize: 15
                font.family: "Microsoft YaHei"
                font.bold: true
                elide: Text.ElideRight      // 窗口变窄时省略号收缩，不与按钮重叠
                clip: true
            }

            Row {
                id: windowButtons
                anchors.right: parent.right; anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                WindowButton {
                    source: "qrc:/TCV3/res/icon/minimize.png"
                    // 与原版一致：最小化即隐藏到系统托盘（托盘双击恢复）
                    onClicked: root.hide()
                }
                WindowButton {
                    source: root.visibility === Window.Maximized
                            ? "qrc:/TCV3/res/icon/normal.png"
                            : "qrc:/TCV3/res/icon/maximize.png"
                    onClicked: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                }
                WindowButton {
                    source: "qrc:/TCV3/res/icon/close.png"
                    hoverColor: theme.dangerHover
                    onClicked: closeDialog.open()
                }
            }
        }

        // ---------- 菜单栏 ----------
        Row {
            id: menuBar
            anchors.top: titleBar.bottom
            anchors.left: parent.left; anchors.right: parent.right
            height: 54
            spacing: 8
            leftPadding: 14

            MenuButton {
                text: qsTr("首页")
                iconOn: "qrc:/TCV3/res/icon/home_fill.png"
                iconOff: "qrc:/TCV3/res/icon/home_line.png"
                active: currentPage === 0
                onClicked: switchPage(0)
            }
            MenuButton {
                text: qsTr("设置")
                iconOn: "qrc:/TCV3/res/icon/settings_fill.png"
                iconOff: "qrc:/TCV3/res/icon/settings_line.png"
                active: currentPage === 1
                onClicked: switchPage(1)
            }
            MenuButton {
                text: qsTr("关于")
                iconOn: "qrc:/TCV3/res/icon/about_fill.png"
                iconOff: "qrc:/TCV3/res/icon/about_line.png"
                active: currentPage === 2
                onClicked: switchPage(2)
            }
        }

        // ---------- 页面容器 ----------
        Item {
            id: pageHost
            anchors.top: menuBar.bottom
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 16
            opacity: 1

// ===================== 首页 =====================
            Item {
                anchors.fill: parent
                visible: currentPage === 0

                // 区域标题（与设置/关于页风格统一）
                Text {
                    id: homeTitle
                    text: qsTr("实时温度")
                    color: theme.textSecondary
                    font.pixelSize: 13
                    anchors.top: parent.top
                    anchors.left: parent.left
                }

                // 温度卡片：横向铺满全宽、等高
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
                        warningIcon: "qrc:/TCV3/res/icon/cpuTemp-red.png"
                    }
                    TempCard {
                        width: (parent.width - parent.spacing) / 2
                        height: 150
                        title: qsTr("GPU 温度")
                        temperature: bridge.gpuTemp
                        warningThreshold: bridge.warningGpu
                        normalIcon: "qrc:/TCV3/res/icon/gpuTemp.png"
                        warningIcon: "qrc:/TCV3/res/icon/gpuTemp-red.png"
                    }
                }

                // BLE 状态面板（通栏，内容垂直居中）
                Rectangle {
                    id: blePanel
                    anchors.top: tempCards.bottom
                    anchors.topMargin: 16
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 84
                    radius: 10
                    color: theme.bgPanel

                    ThemeIcon {
                        id: bleIcon
                        iconWidth: 36; iconHeight: 36
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        source: bridge.bleState === 3 ? "qrc:/TCV3/res/icon/BLE_On.png"
                              : bridge.bleState === 4 ? "qrc:/TCV3/res/icon/BLE_Error.png"
                              : "qrc:/TCV3/res/icon/BLE_Off.png"
                        // 状态语义用颜色表达（白色线稿图标在浅色主题下必须着色）
                        color: bridge.bleState === 3 ? theme.success
                             : bridge.bleState === 4 ? theme.danger : theme.textSecondary
                    }

                    // 标题 + 状态 + 忙指示条（同一行，垂直居中）
                    Row {
                        anchors.left: bleIcon.right; anchors.leftMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 14
                        Column {
                            spacing: 4
                            Text {
                                text: qsTr("蓝牙连接")
                                color: theme.textSecondary; font.pixelSize: 12
                            }
                            Text {
                                text: bridge.bleStatusText
                                color: bridge.bleState === 3 ? theme.success
                                     : bridge.bleState === 4 ? theme.danger : theme.textPrimary
                                font.pixelSize: 15
                            }
                        }
                        Rectangle {
                            id: busyTrack
                            width: 110; height: 4; radius: 2
                            color: theme.bgTrack
                            anchors.verticalCenter: parent.verticalCenter
                            visible: bridge.bleState === 1 || bridge.bleState === 2
                            Rectangle {
                                id: busyBar
                                width: 40; height: parent.height; radius: 2
                                color: theme.accent
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
                        anchors.right: parent.right; anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("重连")
                        onClicked: bridge.reconnectBle()
                        background: Rectangle {
                            radius: 6; color: parent.hovered ? theme.accentHover : theme.accent
                            implicitWidth: 64; implicitHeight: 32
                        }
                        contentItem: Text {
                            text: parent.text; color: theme.textOnAccent
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                // 控制区：风扇模式 + 延时，两个等宽等高面板同一行
                Row {
                    id: controlRow
                    anchors.top: blePanel.bottom
                    anchors.topMargin: 16
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 16

                    // ---- 风扇模式面板 ----
                    Rectangle {
                        width: (parent.width - parent.spacing) / 2
                        height: 112
                        radius: 10
                        color: theme.bgPanel

                        Text {
                            text: qsTr("风扇模式")
                            color: theme.textSecondary; font.pixelSize: 13
                            anchors.top: parent.top; anchors.topMargin: 14
                            anchors.left: parent.left; anchors.leftMargin: 16
                        }
                        Row {
                            anchors.left: parent.left; anchors.leftMargin: 16
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 16
                            spacing: 10
                            FanModeButton {
                                text: qsTr("自动")
                                active: fanMode === 0
                                onClicked: { fanMode = 0; bridge.setFanMode(0); }
                            }
                            FanModeButton {
                                text: qsTr("静音")
                                active: fanMode === 1
                                onClicked: { fanMode = 1; bridge.setFanMode(1); }
                            }
                            FanModeButton {
                                text: qsTr("全速")
                                active: fanMode === 2
                                onClicked: { fanMode = 2; bridge.setFanMode(2); }
                            }
                        }
                    }

                    // ---- 数据发送延时面板 ----
                    Rectangle {
                        width: (parent.width - parent.spacing) / 2
                        height: 112
                        radius: 10
                        color: theme.bgPanel

                        Text {
                            text: qsTr("数据发送延时")
                            color: theme.textSecondary; font.pixelSize: 13
                            anchors.top: parent.top; anchors.topMargin: 14
                            anchors.left: parent.left; anchors.leftMargin: 16
                        }
                        Row {
                            anchors.left: parent.left; anchors.leftMargin: 16
                            anchors.bottom: parent.bottom; anchors.bottomMargin: 16
                            spacing: 10
                            IconButton {
                                source: "qrc:/TCV3/res/icon/minus.png"
                                onClicked: bridge.changeDelay(-1)
                            }
                            Rectangle {
                                width: 72; height: 34; radius: 6; color: theme.bgTrack
                                anchors.verticalCenter: parent.verticalCenter
                                Text {
                                    anchors.centerIn: parent
                                    text: bridge.delayUiSeconds + " s"
                                    color: theme.textPrimary; font.pixelSize: 14
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
                                    radius: 6; color: parent.hovered ? theme.accentHover : theme.accent
                                    implicitWidth: 64; implicitHeight: 32
                                }
                                contentItem: Text {
                                    text: parent.text; color: theme.textOnAccent
                                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                }
                            }
                        }
                    }
                }
            }
            // ===================== 设置页 =====================
            Item {
                anchors.fill: parent
                visible: currentPage === 1

                Text {
                    text: qsTr("设置")
                    color: theme.textSecondary; font.pixelSize: 13
                }

                // ---- 外观：主题模式 ----
                Text {
                    id: appearanceTitle
                    text: qsTr("外观")
                    color: theme.textSecondary; font.pixelSize: 13
                    anchors.top: parent.top; anchors.topMargin: 30
                }
                Rectangle {
                    id: themePanel
                    anchors.top: appearanceTitle.bottom; anchors.topMargin: 10
                    anchors.left: parent.left; anchors.right: parent.right
                    height: 76; radius: 10; color: theme.bgPanel

                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.top: parent.top; anchors.topMargin: 14
                        text: qsTr("主题模式")
                        color: theme.textPrimary; font.pixelSize: 13
                    }
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.bottom: parent.bottom; anchors.bottomMargin: 14
                        text: qsTr("跟随系统或手动切换浅色 / 深色")
                        color: theme.textTertiary; font.pixelSize: 11
                    }
                    Row {
                        anchors.right: parent.right; anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8
                        ThemeButton { text: qsTr("跟随系统"); active: bridge.themeMode === 0; onClicked: bridge.setThemeMode(0) }
                        ThemeButton { text: qsTr("浅色"); active: bridge.themeMode === 1; onClicked: bridge.setThemeMode(1) }
                        ThemeButton { text: qsTr("深色"); active: bridge.themeMode === 2; onClicked: bridge.setThemeMode(2) }
                    }
                }

                // ---- 外观：背景材质 ----
                Rectangle {
                    id: backdropPanel
                    anchors.top: themePanel.bottom; anchors.topMargin: 10
                    anchors.left: parent.left; anchors.right: parent.right
                    height: 76; radius: 10; color: theme.bgPanel

                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.top: parent.top; anchors.topMargin: 14
                        text: qsTr("背景材质")
                        color: theme.textPrimary; font.pixelSize: 13
                    }
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.bottom: parent.bottom; anchors.bottomMargin: 14
                        text: qsTr("Windows 11 云母 (Mica) / 亚克力 (Acrylic)，旧系统自动降级")
                        color: theme.textTertiary; font.pixelSize: 11
                    }
                    Row {
                        anchors.right: parent.right; anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 8
                        ThemeButton { text: qsTr("云母"); active: bridge.backdropType === 1; onClicked: bridge.setBackdropType(1) }
                        ThemeButton { text: qsTr("亚克力"); active: bridge.backdropType === 2; onClicked: bridge.setBackdropType(2) }
                        ThemeButton { text: qsTr("无"); active: bridge.backdropType === 0; onClicked: bridge.setBackdropType(0) }
                    }
                }

                // 开机自启
                Rectangle {
                    id: autostartPanel
                    anchors.top: backdropPanel.bottom; anchors.topMargin: 14
                    anchors.left: parent.left; anchors.right: parent.right
                    height: 64; radius: 10; color: theme.bgPanel

                    ThemeIcon {
                        id: asIcon
                        source: "qrc:/TCV3/res/icon/autoStart.png"
                        iconWidth: 28; iconHeight: 28
                        color: theme.textSecondary
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        anchors.left: asIcon.right; anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("开机自启动")
                        color: theme.textPrimary; font.pixelSize: 14
                    }
                    IconButton {
                        anchors.right: parent.right; anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        width: 56; height: 30
                        source: bridge.autoStartEnabled
                                ? "qrc:/TCV3/res/icon/switch-on.png"
                                : "qrc:/TCV3/res/icon/switch-off.png"
                        onClicked: bridge.toggleAutoStart()
                    }
                }

                // 重置设置
                Rectangle {
                    anchors.top: autostartPanel.bottom; anchors.topMargin: 14
                    anchors.left: parent.left; anchors.right: parent.right
                    height: 64; radius: 10; color: theme.bgPanel

                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("重置设置")
                        color: theme.textPrimary; font.pixelSize: 14
                    }
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 16
                        anchors.top: parent.top; anchors.topMargin: 38
                        text: qsTr("删除配置文件并重启应用")
                        color: theme.textTertiary; font.pixelSize: 11
                    }
                    Button {
                        focusPolicy: Qt.NoFocus
                        anchors.right: parent.right; anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("重置")
                        onClicked: bridge.resetSettings()
                        background: Rectangle {
                            radius: 6; color: parent.hovered ? theme.dangerHover : theme.danger
                            implicitWidth: 60; implicitHeight: 28
                        }
                        contentItem: Text {
                            text: parent.text; color: theme.textOnAccent
                            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            // ===================== 关于页 =====================
            Item {
                anchors.fill: parent
                visible: currentPage === 2

                Text {
                    text: qsTr("关于")
                    color: theme.textSecondary; font.pixelSize: 13
                }

                Rectangle {
                    anchors.top: parent.top; anchors.topMargin: 30
                    anchors.left: parent.left; anchors.right: parent.right
                    height: 110; radius: 10; color: theme.bgPanel

                    // 点击 Logo 触发彩蛋 🥚
                    Item {
                        width: 64; height: 64
                        anchors.left: parent.left; anchors.leftMargin: 20
                        anchors.verticalCenter: parent.verticalCenter
                        ThemeIcon {
                            anchors.fill: parent
                            source: "qrc:/TCV3/res/icon/TreeNetWork_128.png"
                            iconWidth: 64; iconHeight: 64
                            color: theme.accent
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("点击有惊喜 🥚")
                            onClicked: eggDialog.open()
                        }
                    }
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 104
                        anchors.top: parent.top; anchors.topMargin: 20
                        text: qsTr("智能散热小桌板")
                        color: theme.textPrimary; font.pixelSize: 17; font.bold: true
                    }
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 104
                        anchors.top: parent.top; anchors.topMargin: 50
                        text: qsTr("版本 V") + bridge.appVersion
                        color: theme.textSecondary; font.pixelSize: 12
                    }
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 104
                        anchors.top: parent.top; anchors.topMargin: 76
                        text: qsTr("根据电脑温度自动调节风扇转速")
                        color: theme.textTertiary; font.pixelSize: 11
                    }
                    // 资金支持致谢（原 Widgets 版 TCV3.ui 中的 Label，QML 迁移时一并恢复）
                    Text {
                        anchors.left: parent.left; anchors.leftMargin: 104
                        anchors.top: parent.top; anchors.topMargin: 96
                        text: qsTr("感谢 cyjycx 的资金支持！")
                        color: theme.accent; font.pixelSize: 12; font.bold: true
                    }
                }

                Row {
                    anchors.top: parent.top; anchors.topMargin: 160
                    spacing: 14
                    ActionButton {
                        text: qsTr("更新日志")
                        iconSource: "qrc:/TCV3/res/icon/updataLog.png"
                        onClicked: logDialog.open()
                    }
                    ActionButton {
                        text: qsTr("支持与文档")
                        iconSource: "qrc:/TCV3/res/icon/github-line.png"
                        onClicked: bridge.openSupportUrl()
                    }
                }
            }
        }
    }

    // ==================================================================
    // 页面切换动画（淡入淡出）
    // ==================================================================
    function switchPage(target) {
        if (target === currentPage || switching) return
        switching = true
        switchTarget = target
        pageFadeOut.start()
    }

    NumberAnimation {
        id: pageFadeOut
        target: pageHost
        property: "opacity"
        to: 0
        duration: 150
        easing.type: Easing.InOutCubic
        onFinished: {
            currentPage = switchTarget
            pageHost.opacity = 0
            pageFadeIn.start()
        }
    }
    NumberAnimation {
        id: pageFadeIn
        target: pageHost
        property: "opacity"
        to: 1
        duration: 200
        easing.type: Easing.OutCubic
        onFinished: switching = false
    }

    // ==================================================================
    // 退出提示对话框
    // ==================================================================
    Dialog {
        id: closeDialog
        modal: true
        width: 420
        // 显式居中：Popup 不支持 anchors，默认居中逻辑在无边框透明窗口下会失效（落到左上角）
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        standardButtons: Dialog.NoButton
        background: Rectangle { color: theme.bgDialog; radius: 10; border.color: theme.border }

        // 头部：应用图标 + 标题
        header: Rectangle {
            color: theme.bgPanel; radius: 10; height: 50
            ThemeIcon {
                id: closeDlgLogo
                source: "qrc:/TCV3/res/icon/TreeNetWork_128.png"
                iconWidth: 22; iconHeight: 22
                color: theme.accent
                anchors.left: parent.left; anchors.leftMargin: 18
                anchors.verticalCenter: parent.verticalCenter
            }
            Label {
                anchors.left: closeDlgLogo.right; anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("退出提示")
                color: theme.textPrimary
                font.pixelSize: 14
                font.bold: true
            }
        }

        contentItem: Column {
            spacing: 12
            Text {
                text: qsTr("要退出程序，还是最小化到系统托盘？")
                color: theme.textPrimary; font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            Text {
                text: qsTr("最小化到托盘后，程序仍会在后台监控温度，双击托盘图标可恢复窗口。")
                color: theme.textTertiary; font.pixelSize: 11
                wrapMode: Text.WordWrap
                lineHeight: 1.4
            }
            // 按钮排布：危险操作左对齐、主操作右对齐（各与内容区边缘留间距）
            Item {
                width: parent.width
                height: 34
                // 危险操作：退出程序（描边弱化，避免误触）—— 左对齐
                Button {
                    focusPolicy: Qt.NoFocus
                    text: qsTr("退出程序")
                    implicitWidth: 100; implicitHeight: 34
                    anchors.left: parent.left
                    anchors.leftMargin: 2
                    onClicked: { closeDialog.close(); bridge.quitApp(); }
                    background: Rectangle {
                        radius: 6
                        color: "transparent"
                        border.color: parent.hovered ? theme.danger : theme.border
                        border.width: 1
                        implicitWidth: 100; implicitHeight: 34
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.hovered ? theme.danger : theme.textSecondary
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }
                // 主操作：最小化到托盘（accent 实色）—— 右对齐
                Button {
                    focusPolicy: Qt.NoFocus
                    text: qsTr("最小化到托盘")
                    implicitWidth: 140; implicitHeight: 34
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    onClicked: { closeDialog.close(); root.hide(); }
                    background: Rectangle {
                        radius: 6; color: parent.hovered ? theme.accentHover : theme.accent
                        implicitWidth: 140; implicitHeight: 34
                    }
                    contentItem: Text {
                        text: parent.text; color: theme.textOnAccent
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    Dialog {
        id: logDialog
        modal: true
        width: 580
        height: 440
        // 显式居中（Popup 不支持 anchors）
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        standardButtons: Dialog.NoButton
        footer: Rectangle {
            // 固定高度容器：按钮垂直居中、右对齐（footer 直接放 Button 时高度计算不可靠）
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
                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? theme.accentHover : theme.accent
                    implicitWidth: 80; implicitHeight: 30
                }
                contentItem: Text {
                    text: parent.text; color: theme.textOnAccent
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: logDialog.close()
            }
        }
        background: Rectangle { color: theme.bgDialog; radius: 10; border.color: theme.border }
        header: Rectangle {
            color: theme.bgPanel; radius: 10; height: 46
            Label {
                anchors.centerIn: parent
                text: qsTr("更新日志"); color: theme.textPrimary
            }
        }
        contentItem: ScrollView {
            TextArea {
                readOnly: true
                text: bridge.readUpdateLog()
                font.family: "Microsoft YaHei"
                font.pixelSize: 12
                background: Rectangle { color: theme.bgPanel; radius: 6 }
                wrapMode: TextEdit.Wrap
            }
        }
    }

    // ==================================================================
    // 彩蛋对话框（神秘彩蛋 🥚）
    // ==================================================================
    Dialog {
        id: eggDialog
        modal: true
        width: 420
        // 显式居中（Popup 不支持 anchors）
        x: (root.width - width) / 2
        y: (root.height - height) / 2
        standardButtons: Dialog.NoButton
        padding: 20
        background: Rectangle { color: theme.bgDialog; radius: 10; border.color: theme.border }
        header: Rectangle {
            color: theme.bgPanel; radius: 10; height: 46
            Label {
                anchors.centerIn: parent
                text: qsTr("彩蛋"); color: theme.textPrimary
            }
        }
        contentItem: Column {
            spacing: 14
            width: parent.width
            ThemeIcon {
                source: "qrc:/TCV3/res/icon/TreeNetWork_128.png"
                iconWidth: 56; iconHeight: 56
                color: theme.accent
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Text {
                width: parent.width
                text: qsTr("啊~真的Rich!")
                color: theme.accent
                font.pixelSize: 24; font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
            Text {
                width: parent.width
                text: qsTr("感谢所有支持这个项目的朋友！")
                color: theme.textTertiary; font.pixelSize: 12
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
                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? theme.accentHover : theme.accent
                    implicitWidth: 80; implicitHeight: 30
                }
                contentItem: Text {
                    text: parent.text; color: theme.textOnAccent
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: eggDialog.close()
            }
        }
    }

    // ==================================================================
    // 可复用组件
    // ==================================================================

    component WindowButton: Button {
        implicitWidth: 36
        implicitHeight: 30
        property string source: ""
        property color hoverColor: theme.accentSoft
        focusPolicy: Qt.NoFocus
        background: Rectangle {
            radius: 6
            color: parent.hovered ? parent.hoverColor : "transparent"
        }
        contentItem: ThemeIcon {
            source: parent.source
            iconWidth: 16; iconHeight: 16
            color: theme.textPrimary
            anchors.centerIn: parent
        }
    }

    component MenuButton: Button {
        implicitWidth: 96
        implicitHeight: 36
        // 注意：text 用基类 AbstractButton.text（FINAL，不可重写），这里只加图标/激活状态属性
        property string iconOn: ""
        property string iconOff: ""
        property bool active: false
        focusPolicy: Qt.NoFocus
        background: Rectangle {
            radius: 8
            // hover 用 accent 色调高亮（WinUI 风格），两主题下对比度都强
            color: parent.active ? theme.accentSoft : (parent.hovered ? theme.accentSoft : "transparent")
        }
        contentItem: Row {
            spacing: 6
            anchors.centerIn: parent
            ThemeIcon {
                source: parent.parent.active ? parent.parent.iconOn : parent.parent.iconOff
                iconWidth: 18; iconHeight: 18
                color: parent.parent.active ? theme.accent
                     : parent.parent.hovered ? theme.textPrimary : theme.textSecondary
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: parent.parent.text
                color: parent.parent.active ? theme.accent
                     : parent.parent.hovered ? theme.textPrimary : theme.textSecondary
                font.pixelSize: 13
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    component TempCard: Rectangle {
        id: card
        property string title: ""
        property int temperature: 0
        property int warningThreshold: 80
        property string normalIcon: ""
        property string warningIcon: ""
        radius: 10
        color: theme.bgPanel
        readonly property bool warn: temperature > warningThreshold

        // 标题：左上角
        Text {
            text: card.title
            color: theme.textSecondary
            font.pixelSize: 12
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.top: parent.top; anchors.topMargin: 14
        }
        // 图标 + 温度：垂直居中（图标为白色线稿，按状态着色以适配浅色/深色主题）
        ThemeIcon {
            id: iconImg
            source: card.warn ? card.warningIcon : card.normalIcon
            iconWidth: 44; iconHeight: 44
            color: card.warn ? theme.danger : theme.textSecondary
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 4
        }
        Text {
            id: tempText
            text: (card.temperature > 0 ? card.temperature : "--") + " ℃"
            color: card.warn ? theme.danger : theme.textPrimary
            font.pixelSize: 28
            font.bold: true
            anchors.left: iconImg.right; anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 4
        }
        // 进度条：贴底
        ProgressBar {
            id: bar
            anchors.left: parent.left; anchors.leftMargin: 16
            anchors.right: parent.right; anchors.rightMargin: 16
            anchors.bottom: parent.bottom; anchors.bottomMargin: 14
            height: 8
            value: card.temperature > 0 ? card.temperature / 100 : 0
            background: Rectangle { radius: 4; color: theme.bgTrack }
            contentItem: Rectangle {
                radius: 4
                color: card.warn ? theme.danger : theme.success
                width: bar.visualPosition * bar.width
                height: 8
            }
        }
    }

    component FanModeButton: Button {
        implicitWidth: 100
        implicitHeight: 40
        // text 用基类 AbstractButton.text（FINAL）
        property bool active: false
        focusPolicy: Qt.NoFocus
        background: Rectangle {
            radius: 8
            color: parent.active ? theme.accentSoft : (parent.hovered ? theme.accentSoft : theme.bgPanel)
            border.color: parent.active ? theme.accent : (parent.hovered ? theme.accent : theme.border)
            border.width: 1
        }
        contentItem: Text {
            text: parent.text
            color: parent.active ? theme.textPrimary
                 : parent.hovered ? theme.textPrimary : theme.textSecondary
            font.pixelSize: 14
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    component IconButton: Button {
        property string source: ""
        focusPolicy: Qt.NoFocus
        implicitWidth: 34
        implicitHeight: 34
        background: Rectangle {
            radius: 6
            color: parent.hovered ? theme.accentSoft : theme.bgTrack
            border.color: parent.hovered ? theme.accent : theme.border
            border.width: 1
        }
        contentItem: ThemeIcon {
            source: parent.source
            iconWidth: 18; iconHeight: 18
            color: theme.textPrimary
            anchors.centerIn: parent
        }
    }

    component ActionButton: Button {
        // text 用基类 AbstractButton.text（FINAL）；icon 与基类冲突，改用 iconSource
        property string iconSource: ""
        focusPolicy: Qt.NoFocus
        implicitWidth: 150
        implicitHeight: 44
        background: Rectangle {
            radius: 8
            color: parent.hovered ? theme.accentSoft : theme.bgPanel
            border.color: parent.hovered ? theme.accent : theme.border
            border.width: 1
        }
        contentItem: Row {
            spacing: 8
            anchors.centerIn: parent
            ThemeIcon {
                source: parent.parent.iconSource
                iconWidth: 18; iconHeight: 18
                color: parent.parent.hovered ? theme.textPrimary : theme.textSecondary
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: parent.parent.text
                color: theme.textPrimary; font.pixelSize: 13
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    component ThemeButton: Button {
        // text 用基类 AbstractButton.text（FINAL，不可重写）
        property bool active: false
        focusPolicy: Qt.NoFocus
        implicitWidth: 84
        implicitHeight: 30
        background: Rectangle {
            radius: 15
            // 常态 bgTrack 让胶囊圆角始终可见（避免"悬停时才出现圆角"的突变）
            color: parent.active ? theme.accent : (parent.hovered ? theme.accentSoft : theme.bgTrack)
            border.color: parent.active ? theme.accent : (parent.hovered ? theme.accent : theme.border)
            border.width: 1
        }
        contentItem: Text {
            text: parent.text
            color: parent.active ? theme.textOnAccent
                 : parent.hovered ? theme.textPrimary : theme.textSecondary
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // 主题化图标：对单色线稿图标着色，随主题明暗自动变色（保留 alpha 通道）
    component ThemeIcon: Item {
        property string source: ""
        property color color: theme.textSecondary
        property int iconWidth: 18
        property int iconHeight: 18
        implicitWidth: iconWidth
        implicitHeight: iconHeight

        // 图标内容固定为 iconWidth x iconHeight，并在父级中居中。
        // 关键：ThemeIcon 作为 Button 的 contentItem 时会被按钮拉伸到内容区，
        // 若 Image/MultiEffect 跟随父级尺寸，会出现"一个正常缩放 + 一个拉伸"的重叠；
        // 这里 Image 与 MultiEffect 同尺寸完全重合（MultiEffect 覆盖在上层着色），
        // 无论父级被拉伸多大，图标始终不变形、不重复
        Image {
            id: themeIconSrc
            source: parent.source
            width: parent.iconWidth
            height: parent.iconHeight
            sourceSize.width: parent.iconWidth * 2   // 2x 解码保证清晰
            sourceSize.height: parent.iconHeight * 2
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
        }
        MultiEffect {
            anchors.fill: themeIconSrc
            source: themeIconSrc
            colorization: 1.0
            colorizationColor: parent.color
        }
    }
}
