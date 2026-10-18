import QtQuick
import QtQuick.Controls

Rectangle {
    id: card
    property string title: ""
    property int temperature: 0
    property int warningThreshold: 80
    // 图标类型："cpu" | "gpu"（原来靠传两张 PNG 路径 + 显示/隐藏切换，
    // 现在图标是 QML 矢量的 IconChipTemp，同一实例只换描边颜色即可）
    property string iconKind: "cpu"
    radius: AppTheme.radiusPanel
    color: AppTheme.bgPanel
    readonly property bool warn: temperature > warningThreshold

    // NASA 朋克：1px 金属细边勾勒「骨架」
    border.width: AppTheme.nasa ? 1 : 0
    border.color: AppTheme.border

    readonly property int iconSize: 44
    readonly property int iconLeft: 16
    readonly property int textGap: 14

    Text {
        text: AppTheme.nasa ? card.title.toUpperCase() : card.title
        color: AppTheme.textSecondary
        font.pixelSize: AppTheme.nasa ? 10 : 12
        font.family: AppTheme.nasa ? AppTheme.fontMono : AppTheme.fontBody
        font.letterSpacing: AppTheme.nasa ? 1.2 : 0
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.top: parent.top
        anchors.topMargin: 14
    }

    // 温度图标：QML 矢量绘制（替代 cpuTemp.png / gpuTemp.png），
    // 报警时只换描边色，不再需要「正常/报警两份图标」来回切
    IconChipTemp {
        id: cardIcon
        kind: card.iconKind
        size: card.iconSize
        color: card.warn ? AppTheme.danger
                         : (AppTheme.nasa ? AppTheme.nasaMetalMid : AppTheme.textSecondary)
        anchors.left: parent.left
        anchors.leftMargin: card.iconLeft
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 4
    }

    Text {
        id: tempText
        text: (card.temperature > 0 ? card.temperature : "--") + " ℃"
        color: card.warn ? AppTheme.danger : AppTheme.textPrimary
        font.pixelSize: AppTheme.nasa ? 30 : 28
        font.bold: !AppTheme.nasa
        font.family: AppTheme.nasa ? AppTheme.fontDisplay : AppTheme.fontBody
        anchors.left: parent.left
        anchors.leftMargin: card.iconLeft + card.iconSize + card.textGap
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: AppTheme.nasa ? -2 : 4
    }

    // NASA 朋克：读数下方的细刻度条
    NasaTicks {
        anchors.left: tempText.left
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.top: tempText.bottom
        anchors.topMargin: 1
        height: 7
        tickCount: 40
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

        background: Rectangle { radius: AppTheme.radiusTick; color: AppTheme.bgTrack }

        contentItem: Rectangle {
            radius: AppTheme.radiusTick
            color: card.warn ? AppTheme.danger : AppTheme.success
            width: bar.visualPosition * bar.width
            height: 8
        }
    }

    // NASA 朋克：四角铆钉
    NasaRivets {
        anchors.fill: parent
        rivetInset: 7
        rivetSize: 5
    }
}
