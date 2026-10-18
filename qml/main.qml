import QtQuick
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

    property bool systemDark: bridge.systemDark
    // 主题由「风格 × 模式」两个维度决定，交给 AppTheme 推导：
    //   风格 0=默认 1=NASA 朋克；模式 0=跟随系统 1=手动·浅色 2=手动·深色
    readonly property bool isDark: AppTheme.isDark

    Binding {
        target: AppTheme
        property: "style"
        value: bridge.themeStyle
    }

    Binding {
        target: AppTheme
        property: "mode"
        value: bridge.themeMode
    }

    Binding {
        target: AppTheme
        property: "systemDark"
        value: root.systemDark
    }

    onIsDarkChanged: bridge.applyDarkMode(isDark)

    Component.onCompleted: {
        console.log("themeStyle:", bridge.themeStyle, " themeMode:", bridge.themeMode)
        console.log("isDark:", isDark, " nasa:", AppTheme.nasa)
        console.log("bridge.systemDark:", bridge.systemDark)
        bridge.applyDarkMode(isDark)
    }

    onClosing: (close) => {
        close.accepted = false
        appWindow.openCloseDialog()
    }

    Connections {
        target: bridge
        function onRestoreRequested() {
            root.show()
            root.raise()
        }
    }

    AppWindow {
        id: appWindow
        anchors.fill: parent
    }
}