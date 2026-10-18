import QtQuick
import QtQuick.Controls

Button {
    id: control
    implicitWidth: 36
    implicitHeight: 30
    property string source: ""
    property color hoverColor: AppTheme.accentSoft
    // 图标色可覆盖：NASA 模式的标题栏是深色仪表板，深灰图标会看不见
    property color iconColor: AppTheme.textPrimary
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: AppTheme.radiusButton
        color: control.hovered ? control.hoverColor : "transparent"
    }

    contentItem: ThemeIcon {
        source: control.source
        iconWidth: 16
        iconHeight: 16
        color: control.iconColor
        anchors.centerIn: parent
    }
}
