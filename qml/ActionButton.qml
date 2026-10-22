import QtQuick
import QtQuick.Controls

Button {
    id: control
    property string iconSource: ""
    /// 可选：用 QML 矢量图标（如 IconDownload）代替位图 iconSource。
    /// 传了它就不再显示 iconSource 的位图。
    property Component iconComponent: null
    focusPolicy: Qt.NoFocus
    implicitWidth: 150
    implicitHeight: 44

    background: Rectangle {
        radius: AppTheme.radiusSurface
        color: control.hovered ? AppTheme.accentSoft : AppTheme.bgPanel
        border.color: control.hovered ? AppTheme.accent : AppTheme.border
        border.width: 1
    }

    contentItem: Row {
        spacing: 8
        anchors.centerIn: parent

        Loader {
            anchors.verticalCenter: parent.verticalCenter
            visible: control.iconComponent !== null
            sourceComponent: control.iconComponent
        }

        ThemeIcon {
            source: control.iconSource
            iconWidth: 18
            iconHeight: 18
            visible: control.iconComponent === null
            color: control.hovered ? AppTheme.textPrimary : AppTheme.textSecondary
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: control.text
            color: AppTheme.textPrimary
            font.family: AppTheme.fontBody
            font.pixelSize: 13
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}