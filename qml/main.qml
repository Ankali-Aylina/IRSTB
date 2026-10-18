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
    // 主题模式（0=跟随系统 1=浅色 2=深色 3=NASA 朋克）与系统深浅色都交给 AppTheme 推导，
    // 这样 AppTheme 能同时决定 isDark 和 nasa 两个派生状态。
    readonly property bool isDark: AppTheme.isDark

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
        console.log("themeMode:", bridge.themeMode)
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