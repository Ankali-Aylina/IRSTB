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
    property bool isDark: bridge.themeMode === 1 ? false
                       : bridge.themeMode === 2 ? true
                       : systemDark

    Binding {
        target: AppTheme
        property: "isDark"
        value: root.isDark
    }

    onIsDarkChanged: bridge.applyDarkMode(isDark)

    Component.onCompleted: {
        console.log("isDark:", isDark)
        console.log("bridge.themeMode:", bridge.themeMode)
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