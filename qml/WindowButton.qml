import QtQuick
import QtQuick.Controls

Button {
    id: control
    implicitWidth: 36
    implicitHeight: 30
    property string source: ""
    property color hoverColor: AppTheme.accentSoft
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: 6
        color: control.hovered ? control.hoverColor : "transparent"
    }

    contentItem: ThemeIcon {
        source: control.source
        iconWidth: 16
        iconHeight: 16
        color: AppTheme.textPrimary
        anchors.centerIn: parent
    }
}