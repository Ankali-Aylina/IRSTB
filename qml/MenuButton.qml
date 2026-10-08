import QtQuick
import QtQuick.Controls

Button {
    id: control
    implicitWidth: 96
    implicitHeight: 36
    property string iconOn: ""
    property string iconOff: ""
    property bool active: false
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: 8
        color: control.active ? AppTheme.accentSoft
             : control.hovered ? AppTheme.accentSoft
             : "transparent"
    }

    contentItem: Row {
        spacing: 6
        anchors.centerIn: parent

        ThemeIcon {
            source: control.active ? control.iconOn : control.iconOff
            iconWidth: 18
            iconHeight: 18
            color: control.active ? AppTheme.accent
                 : control.hovered ? AppTheme.textPrimary
                 : AppTheme.textSecondary
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: control.text
            color: control.active ? AppTheme.accent
                 : control.hovered ? AppTheme.textPrimary
                 : AppTheme.textSecondary
            font.pixelSize: 13
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}