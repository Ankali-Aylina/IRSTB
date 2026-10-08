pragma Singleton
import QtQuick

QtObject {
    id: theme

    property bool isDark: false

    // 浅色主题
    readonly property color lightBgBase: "#E6F3F3F3"
    readonly property color lightBgPanel: "#B3FFFFFF"
    readonly property color lightBgTrack: "#E5E5E5"
    readonly property color lightBgHover: "#D9E5E5E5"
    readonly property color lightBgDialog: "#F2FFFFFF"
    readonly property color lightBorder: "#1F000000"
    readonly property color lightTextPrimary: "#1B1B1B"
    readonly property color lightTextSecondary: "#616161"
    readonly property color lightTextTertiary: "#8A8A8A"
    readonly property color lightAccent: "#0067C0"
    readonly property color lightAccentHover: "#005BB8"
    readonly property color lightAccentSoft: "#E6F0F9"
    readonly property color lightTextOnAccent: "#FFFFFF"
    readonly property color lightDanger: "#C42B1C"
    readonly property color lightDangerHover: "#A82417"
    readonly property color lightDangerSoft: "#F7E4E1"
    readonly property color lightSuccess: "#0F7B0F"
    // 进行中（连接中 / 重连中）的天蓝色
    readonly property color lightInfo: "#0284C7"

    // 深色主题
    readonly property color darkBgBase: "#E6202020"
    readonly property color darkBgPanel: "#B32B2B2B"
    readonly property color darkBgTrack: "#3D3D3D"
    readonly property color darkBgHover: "#4A4A4A"
    readonly property color darkBgDialog: "#F22B2B2B"
    readonly property color darkBorder: "#1FFFFFFF"
    readonly property color darkTextPrimary: "#FFFFFF"
    readonly property color darkTextSecondary: "#A8A8A8"
    readonly property color darkTextTertiary: "#8A8A8A"
    readonly property color darkAccent: "#4CC2FF"
    readonly property color darkAccentHover: "#5FC9FF"
    readonly property color darkAccentSoft: "#4A6A7A"
    readonly property color darkTextOnAccent: "#000000"
    readonly property color darkDanger: "#FF99A4"
    readonly property color darkDangerHover: "#FFB3BB"
    readonly property color darkDangerSoft: "#3D2A2C"
    readonly property color darkSuccess: "#6CCB5F"
    // 深色下的天蓝（与 darkAccent 同色，保证与搜索/连接进度条颜色一致）
    readonly property color darkInfo: "#4CC2FF"

    // 当前主题
    readonly property color bgBase: isDark ? darkBgBase : lightBgBase
    readonly property color bgPanel: isDark ? darkBgPanel : lightBgPanel
    readonly property color bgTrack: isDark ? darkBgTrack : lightBgTrack
    readonly property color bgHover: isDark ? darkBgHover : lightBgHover
    readonly property color bgDialog: isDark ? darkBgDialog : lightBgDialog
    readonly property color border: isDark ? darkBorder : lightBorder
    readonly property color textPrimary: isDark ? darkTextPrimary : lightTextPrimary
    readonly property color textSecondary: isDark ? darkTextSecondary : lightTextSecondary
    readonly property color textTertiary: isDark ? darkTextTertiary : lightTextTertiary
    readonly property color accent: isDark ? darkAccent : lightAccent
    readonly property color accentHover: isDark ? darkAccentHover : lightAccentHover
    readonly property color accentSoft: isDark ? darkAccentSoft : lightAccentSoft
    readonly property color textOnAccent: isDark ? darkTextOnAccent : lightTextOnAccent
    readonly property color danger: isDark ? darkDanger : lightDanger
    readonly property color dangerHover: isDark ? darkDangerHover : lightDangerHover
    readonly property color dangerSoft: isDark ? darkDangerSoft : lightDangerSoft
    readonly property color success: isDark ? darkSuccess : lightSuccess
    readonly property color info: isDark ? darkInfo : lightInfo
}