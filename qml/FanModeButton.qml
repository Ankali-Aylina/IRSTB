import QtQuick
import QtQuick.Controls

Button {
    id: control
    implicitWidth: 100
    implicitHeight: 40
    property bool active: false
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: 8
        color: control.active ? AppTheme.accentSoft
             : control.hovered ? AppTheme.accentSoft
             : AppTheme.bgPanel
        border.color: control.active ? AppTheme.accent
                    : control.hovered ? AppTheme.accent
                    : AppTheme.border
        border.width: 1
    }

    contentItem: Text {
        text: control.text
        color: control.active ? AppTheme.textPrimary
             : control.hovered ? AppTheme.textPrimary
             : AppTheme.textSecondary
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
