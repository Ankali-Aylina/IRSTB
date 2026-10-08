import QtQuick
import QtQuick.Effects

Item {
    id: themeIconRoot
    property string source: ""
    property color color: AppTheme.textSecondary
    property int iconWidth: 18
    property int iconHeight: 18

    width: iconWidth
    height: iconHeight
    implicitWidth: iconWidth
    implicitHeight: iconHeight

    Image {
        id: themeIconSrc
        anchors.fill: parent
        source: themeIconRoot.source
        sourceSize.width: themeIconRoot.iconWidth * 2
        sourceSize.height: themeIconRoot.iconHeight * 2
        fillMode: Image.PreserveAspectFit

        layer.enabled: true
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: themeIconRoot.color
        }
    }
}