import QtQuick

Row {
    id: navBar
    height: 54
    spacing: 8
    leftPadding: 14

    property int currentPage: 0
    signal pageClicked(int page)

    MenuButton {
        text: qsTr("首页")
        iconOn: "qrc:/TCV3/res/icon/home_fill.png"
        iconOff: "qrc:/TCV3/res/icon/home_line.png"
        active: navBar.currentPage === 0
        onClicked: navBar.pageClicked(0)
    }

    MenuButton {
        text: qsTr("设置")
        iconOn: "qrc:/TCV3/res/icon/settings_fill.png"
        iconOff: "qrc:/TCV3/res/icon/settings_line.png"
        active: navBar.currentPage === 1
        onClicked: navBar.pageClicked(1)
    }

    MenuButton {
        text: qsTr("关于")
        iconOn: "qrc:/TCV3/res/icon/about_fill.png"
        iconOff: "qrc:/TCV3/res/icon/about_line.png"
        active: navBar.currentPage === 2
        onClicked: navBar.pageClicked(2)
    }
}
