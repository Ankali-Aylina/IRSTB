import QtQuick
import QtQuick.Controls

Rectangle {
    id: card
    property string title: ""
    property int temperature: 0
    property int warningThreshold: 80
    property string normalIcon: ""
    property string warningIcon: ""
    radius: 10
    color: AppTheme.bgPanel
    readonly property bool warn: temperature > warningThreshold

    readonly property int iconSize: 44
    readonly property int iconLeft: 16
    readonly property int textGap: 14

    Text {
        text: card.title
        color: AppTheme.textSecondary
        font.pixelSize: 12
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.top: parent.top
        anchors.topMargin: 14
    }

    ThemeIcon {
        id: iconImgNormal
        source: card.normalIcon
        iconWidth: card.iconSize
        iconHeight: card.iconSize
        color: AppTheme.textSecondary
        anchors.left: parent.left
        anchors.leftMargin: card.iconLeft
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 4
        visible: !card.warn
    }

    ThemeIcon {
        id: iconImgWarn
        source: card.warningIcon
        iconWidth: card.iconSize
        iconHeight: card.iconSize
        color: AppTheme.danger
        anchors.left: parent.left
        anchors.leftMargin: card.iconLeft
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 4
        visible: card.warn
    }

    Text {
        id: tempText
        text: (card.temperature > 0 ? card.temperature : "--") + " ℃"
        color: card.warn ? AppTheme.danger : AppTheme.textPrimary
        font.pixelSize: 28
        font.bold: true
        anchors.left: parent.left
        anchors.leftMargin: card.iconLeft + card.iconSize + card.textGap
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 4
    }

    ProgressBar {
        id: bar
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 14
        height: 8
        value: card.temperature > 0 ? card.temperature / 100 : 0

        background: Rectangle { radius: 4; color: AppTheme.bgTrack }

        contentItem: Rectangle {
            radius: 4
            color: card.warn ? AppTheme.danger : AppTheme.success
            width: bar.visualPosition * bar.width
            height: 8
        }
    }
}