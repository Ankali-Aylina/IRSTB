pragma Singleton
import QtQuick

// 主题中枢。除了颜色，也承担「样式令牌」（圆角、字体、边框）——
// 否则像 NASA 朋克这种需要方角 + 等宽字体的风格就没法只靠换颜色实现。
QtObject {
    id: theme

    // 主题模式：0=跟随系统 1=浅色 2=深色 3=NASA 朋克
    property int mode: 0
    property bool systemDark: false

    readonly property bool nasa: mode === 3
    readonly property bool isDark: nasa ? false
                                 : mode === 1 ? false
                                 : mode === 2 ? true
                                 : systemDark

    // ================= 浅色主题 =================
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

    // ================= 深色主题 =================
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

    // ================= NASA 朋克原始色板 =================
    // 1960-80 年代 NASA 航天器工业设计与技术文档配色。
    // 克制使用：信号红只做点睛，Kapton 金只出现在 1~2 个焦点元素上。
    readonly property color nasaEnamel:     "#F4F3EF"   // 烤漆白（页面主底）
    readonly property color nasaPanel:      "#FBFAF7"   // 面板（比页面略亮一档）
    readonly property color nasaPanelDeep:  "#E9E6DE"   // 凹陷/内槽
    readonly property color nasaPanelHover: "#E2DED4"   // 面板 hover
    readonly property color nasaBlue:       "#0B3D91"   // NASA 蓝（蓝图 / 深色结构）
    readonly property color nasaBlueHover:  "#0E48A8"
    readonly property color nasaBlueDeep:   "#072A66"   // 更深的结构底
    readonly property color nasaBlueSoft:   "#DFE5F2"   // 蓝色浅底（选中态）
    readonly property color nasaMetalLite:  "#A8ACAF"   // 金属灰-亮（边框）
    readonly property color nasaMetalMid:   "#6E7378"   // 金属灰-中（次级文字/刻度）
    readonly property color nasaMetalDark:  "#43484C"   // 金属灰-暗（主文字/骨架）
    readonly property color nasaRed:        "#E4002B"   // 信号红（仅关键警示）
    readonly property color nasaRedHover:   "#C0001F"
    readonly property color nasaRedSoft:    "#F6DEE1"
    readonly property color nasaOrange:     "#C96A2B"   // 火箭橙
    readonly property color nasaOrangeDeep: "#8C4A22"   // 锈蚀橙
    readonly property color nasaGold:       "#E8B84B"   // Kapton 金
    readonly property color nasaGoldDeep:   "#C9932E"
    readonly property color nasaGreen:      "#2C6E49"   // 深绿（仅 GO / 正常）

    // ================= 当前主题 =================
    readonly property color bgBase:     nasa ? nasaEnamel  : (isDark ? darkBgBase : lightBgBase)
    readonly property color bgPanel:    nasa ? nasaPanel   : (isDark ? darkBgPanel : lightBgPanel)
    readonly property color bgTrack:    nasa ? nasaPanelDeep : (isDark ? darkBgTrack : lightBgTrack)
    readonly property color bgHover:    nasa ? nasaPanelHover : (isDark ? darkBgHover : lightBgHover)
    readonly property color bgDialog:   nasa ? "#FFFDFCF9" : (isDark ? darkBgDialog : lightBgDialog)
    readonly property color border:     nasa ? nasaMetalLite : (isDark ? darkBorder : lightBorder)
    readonly property color textPrimary:   nasa ? nasaMetalDark : (isDark ? darkTextPrimary : lightTextPrimary)
    readonly property color textSecondary: nasa ? nasaMetalMid  : (isDark ? darkTextSecondary : lightTextSecondary)
    readonly property color textTertiary:  nasa ? "#8B8F93"     : (isDark ? darkTextTertiary : lightTextTertiary)
    readonly property color accent:      nasa ? nasaBlue      : (isDark ? darkAccent : lightAccent)
    readonly property color accentHover: nasa ? nasaBlueHover : (isDark ? darkAccentHover : lightAccentHover)
    readonly property color accentSoft:  nasa ? nasaBlueSoft  : (isDark ? darkAccentSoft : lightAccentSoft)
    readonly property color textOnAccent: nasa ? "#FFFFFF"     : (isDark ? darkTextOnAccent : lightTextOnAccent)
    readonly property color danger:      nasa ? nasaRed       : (isDark ? darkDanger : lightDanger)
    readonly property color dangerHover: nasa ? nasaRedHover  : (isDark ? darkDangerHover : lightDangerHover)
    readonly property color dangerSoft:  nasa ? nasaRedSoft   : (isDark ? darkDangerSoft : lightDangerSoft)
    readonly property color success:     nasa ? nasaGreen     : (isDark ? darkSuccess : lightSuccess)
    // 进行中：NASA 模式下用火箭橙，与蓝图蓝形成对比且不引入粉彩
    readonly property color info:        nasa ? nasaOrange    : (isDark ? darkInfo : lightInfo)

    // ================= 样式令牌：圆角 =================
    // NASA 朋克要求「矩形、无大圆角（0~2px）」，其余主题保持原有圆润感。
    readonly property int radiusPanel:   nasa ? 0 : 10
    readonly property int radiusSurface: nasa ? 0 : 8
    readonly property int radiusButton:  nasa ? 2 : 6
    readonly property int radiusPill:    nasa ? 2 : 15
    readonly property int radiusTick:    nasa ? 1 : 4
    // 开关轨道：NASA 用方角矩形，其余主题保持胶囊
    readonly property int radiusSwitch:  nasa ? 2 : 9
    // 面板边框：NASA 用 1px 金属细边突出「骨架」感
    readonly property int borderWidth:   1

    // ================= 样式令牌：字体 =================
    // 中文正文始终用雅黑（Nasalization 无中日韩字形，直接套会导致回退不一致）。
    readonly property string fontBody:    "Microsoft YaHei"
    // 拉丁字标 / 大号读数：NASA 模式用 Nasalization Rg
    readonly property string fontDisplay: nasa ? "Nasalization Rg" : "Microsoft YaHei"
    // 技术标签 / 元数据 / 数值：等宽
    readonly property string fontMono:    nasa ? "Consolas" : "Microsoft YaHei"
    readonly property int letterSpacingTight: nasa ? 0.5 : 0

    // ================= NASA 装饰用色 =================
    readonly property color decoRivet:     nasaGoldDeep
    readonly property color decoSeam:      nasaMetalLite
    readonly property color decoGridMajor: "#33FFFFFF"
    readonly property color decoGridMinor: "#1FFFFFFF"
    readonly property color decoHazardA:   "#1A1C1E"
    readonly property color decoHazardB:   nasaGold
}
