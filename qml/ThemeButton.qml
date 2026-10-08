import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool active: false
    focusPolicy: Qt.NoFocus
    implicitWidth: 84
    implicitHeight: 30

    background: Rectangle {
        radius: 15
        color: control.active ? AppTheme.accent
             : control.hovered ? AppTheme.accentSoft
             : AppTheme.bgTrack
        border.color: control.active ? AppTheme.accent
                    : control.hovered ? AppTheme.accent
                    : AppTheme.border
        border.width: 1
    }

    contentItem: Text {
        text: control.text
        color: control.active ? AppTheme.textOnAccent
             : control.hovered ? AppTheme.textPrimary
             : AppTheme.textSecondary
        font.pixelSize: 12
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
