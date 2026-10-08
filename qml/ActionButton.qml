import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string iconSource: ""
    focusPolicy: Qt.NoFocus
    implicitWidth: 150
    implicitHeight: 44

    background: Rectangle {
        radius: 8
        color: control.hovered ? AppTheme.accentSoft : AppTheme.bgPanel
        border.color: control.hovered ? AppTheme.accent : AppTheme.border
        border.width: 1
    }

    contentItem: Row {
        spacing: 8
        anchors.centerIn: parent

        ThemeIcon {
            source: control.iconSource
            iconWidth: 18
            iconHeight: 18
            color: control.hovered ? AppTheme.textPrimary : AppTheme.textSecondary
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: control.text
            color: AppTheme.textPrimary
            font.pixelSize: 13
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}