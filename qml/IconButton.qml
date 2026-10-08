import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string source: ""
    focusPolicy: Qt.NoFocus
    implicitWidth: 34
    implicitHeight: 34

    background: Rectangle {
        radius: 6
        color: control.hovered ? AppTheme.accentSoft : AppTheme.bgTrack
        border.color: control.hovered ? AppTheme.accent : AppTheme.border
        border.width: 1
    }

    contentItem: ThemeIcon {
        source: control.source
        iconWidth: 18
        iconHeight: 18
        color: AppTheme.textPrimary
        anchors.centerIn: parent
    }
}