import QtQuick
import QtQuick.Controls

Rectangle {
    id: titleBar
    // NASA 模式下顶部多出一条「文档控制条」，因此整体加高
    height: AppTheme.nasa ? 58 : 44
    color: AppTheme.nasa ? AppTheme.nasaMetalDark : AppTheme.bgPanel
    radius: AppTheme.radiusSurface

    signal closeRequested()

    // NASA 朋克：顶部细金属文档控制条（等宽小字元数据 + 一小段黄黑危险斜纹）
    Rectangle {
        id: docStrip
        visible: AppTheme.nasa
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 15
        color: "#1A1C1E"

        // 黄黑斜纹（面积刻意很小，只做点缀）
        Canvas {
            id: hazard
            width: 44
            height: parent.height
            anchors.left: parent.left
            anchors.top: parent.top
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.fillStyle = AppTheme.nasaGold
                ctx.fillRect(0, 0, width, height)
                ctx.fillStyle = "#1A1C1E"
                var step = 8
                for (var x = -height; x < width + height; x += step * 2) {
                    ctx.beginPath()
                    ctx.moveTo(x, height)
                    ctx.lineTo(x + step, height)
                    ctx.lineTo(x + step + height, 0)
                    ctx.lineTo(x + height, 0)
                    ctx.closePath()
                    ctx.fill()
                }
            }
            Component.onCompleted: requestPaint()
        }

        Text {
            anchors.left: hazard.right
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: "DOC NO. IRSTB-TC   ·   REV " + bridge.appVersion
                  + "   ·   PAGE 01 / 03   ·   THERMAL CONTROL ASSY."
            color: AppTheme.nasaMetalLite
            font.family: AppTheme.fontMono
            font.pixelSize: 9
            font.letterSpacing: 0.8
            elide: Text.ElideRight
            anchors.right: parent.right
            anchors.rightMargin: 10
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: Window.window.startSystemMove()
    }

    ThemeIcon {
        id: appLogo
        source: "qrc:/TCV3/res/icon/TreeNetWork_128.png"
        iconWidth: 26
        iconHeight: 26
        // NASA 模式：字标用 Kapton 金，是全界面仅有的 1~2 个金色焦点之一
        color: AppTheme.nasa ? AppTheme.nasaGold : AppTheme.accent
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: AppTheme.nasa ? 7 : 0
    }

    Text {
        id: titleText
        anchors.left: appLogo.right
        anchors.leftMargin: 10
        anchors.right: windowButtons.left
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: AppTheme.nasa ? 7 : 0
        text: qsTr("智能散热小桌板")
        color: AppTheme.nasa ? AppTheme.nasaEnamel : AppTheme.textPrimary
        font.pixelSize: 15
        font.family: AppTheme.fontBody
        font.bold: true
        font.letterSpacing: AppTheme.nasa ? 1.5 : 0
        elide: Text.ElideRight
        clip: true
    }

    Row {
        id: windowButtons
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: AppTheme.nasa ? 7 : 0
        spacing: 8

        WindowButton {
            source: "qrc:/TCV3/res/icon/minimize.png"
            iconColor: AppTheme.nasa ? AppTheme.nasaEnamel : AppTheme.textPrimary
            hoverColor: AppTheme.nasa ? "#33FFFFFF" : AppTheme.accentSoft
            onClicked: Window.window.hide()
        }

        WindowButton {
            source: Window.window.visibility === Window.Maximized
                    ? "qrc:/TCV3/res/icon/normal.png"
                    : "qrc:/TCV3/res/icon/maximize.png"
            iconColor: AppTheme.nasa ? AppTheme.nasaEnamel : AppTheme.textPrimary
            hoverColor: AppTheme.nasa ? "#33FFFFFF" : AppTheme.accentSoft
            onClicked: {
                if (Window.window.visibility === Window.Maximized)
                    Window.window.showNormal()
                else
                    Window.window.showMaximized()
            }
        }

        WindowButton {
            source: "qrc:/TCV3/res/icon/close.png"
            iconColor: AppTheme.nasa ? AppTheme.nasaEnamel : AppTheme.textPrimary
            hoverColor: AppTheme.nasa ? AppTheme.nasaRed : AppTheme.dangerHover
            onClicked: titleBar.closeRequested()
        }
    }
}
