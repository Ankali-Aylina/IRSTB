import QtQuick
import QtQuick.Controls

Rectangle {
    id: appWindow
    radius: 8
    color: AppTheme.bgBase
    border.color: AppTheme.border
    border.width: 1
    clip: true

    property int currentPage: 0
    property int switchTarget: 0
    property bool switching: false
    property int fanMode: 0

    function switchPage(target) {
        if (target === currentPage || switching)
            return
        switching = true
        switchTarget = target
        pageFadeOut.start()
    }

    function openCloseDialog() {
        closeDialog.open()
    }

    TitleBar {
        id: titleBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        onCloseRequested: appWindow.openCloseDialog()
    }

    NavigationBar {
        id: navBar
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        currentPage: appWindow.currentPage
        onPageClicked: (page) => appWindow.switchPage(page)
    }

    Item {
        id: pageHost
        anchors.top: navBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        opacity: 1

        HomePage {
            anchors.fill: parent
            visible: appWindow.currentPage === 0
            fanMode: appWindow.fanMode
            onFanModeRequested: (mode) => {
                appWindow.fanMode = mode
                bridge.setFanMode(mode)
            }
        }

        SettingsPage {
            anchors.fill: parent
            visible: appWindow.currentPage === 1
        }

        AboutPage {
            anchors.fill: parent
            visible: appWindow.currentPage === 2
            onShowLogDialog: logDialog.open()
            onShowEggDialog: eggDialog.open()
        }
    }

    NumberAnimation {
        id: pageFadeOut
        target: pageHost
        property: "opacity"
        to: 0
        duration: 150
        easing.type: Easing.InOutCubic
        onFinished: {
            appWindow.currentPage = appWindow.switchTarget
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
        onFinished: appWindow.switching = false
    }

    CloseDialog { id: closeDialog }
    LogDialog { id: logDialog }
    EggDialog { id: eggDialog }
}