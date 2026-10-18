pragma Singleton
import QtQuick

// 主题中枢。除了颜色，也承担「样式令牌」（圆角、字体、边框）——
// 否则像 NASA 朋克这种需要方角 + 等宽字体的风格就没法只靠换颜色实现。
QtObject {
    id: theme

    // 主题模式：0=跟随系统 1=浅色 2=深色 3=NASA 朋克（亮） 4=NASA 朋克（暗）
    property int mode: 0
    property bool systemDark: false

    readonly property bool nasaLight: mode === 3
    readonly property bool nasaDark: mode === 4
    // 「是不是 NASA 风格」的判定（结构决定：方角、等宽字体、金属边框、装饰件）
    readonly property bool nasa: nasaLight || nasaDark
    readonly property bool isDark: nasaDark ? true
                                 : nasaLight ? false
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

    // ================= NASA 朋克 · 暗（夜间仪表板）=================
    // 同一套语言，把明暗关系反过来：深色仪表板底 + 提亮的金属边。
    // 蓝图蓝 / 信号红 / 火箭橙 / Kapton 金都在暗底上做了提亮，否则原色对比不足。
    readonly property color nasaDPanel:      "#1D2228"   // 面板（比底略亮）
    readonly property color nasaDPanelDeep:  "#12161A"   // 凹陷/内槽
    readonly property color nasaDPanelHover: "#262C33"   // 面板 hover
    readonly property color nasaDChrome:     "#2B3238"   // 标题栏仪表板
    readonly property color nasaDChromeDeep: "#0C0E10"   // 文档控制条
    readonly property color nasaDMetal:      "#79828A"   // 金属灰（边框）
    readonly property color nasaDMetalLite:  "#A8B2BA"   // 金属灰-亮
    readonly property color nasaDTextPri:    "#E8EDF2"   // 主文字（烤漆白偏冷）
    readonly property color nasaDTextSec:    "#9AA5AE"   // 次级文字
    readonly property color nasaDTextTer:    "#6E7881"   // 三级文字
    readonly property color nasaDBlue:       "#4A82C8"   // 蓝图蓝（提亮）
    readonly property color nasaDBlueHover:  "#5D93D6"
    readonly property color nasaDBlueSoft:   "#1E2C42"   // 蓝色深底（选中态）
    readonly property color nasaDRed:        "#E9565F"   // 信号红（提亮）
    readonly property color nasaDRedHover:   "#F0757C"
    readonly property color nasaDRedSoft:    "#3A2024"
    readonly property color nasaDOrange:     "#D9803F"   // 火箭橙（提亮）
    readonly property color nasaDGold:       "#E8B84B"   // Kapton 金
    readonly property color nasaDGoldDeep:   "#D2A040"
    readonly property color nasaDGreen:      "#5AA073"   // GO 绿（提亮）

    // ================= 当前主题 =================
    readonly property color bgBase:     nasaDark ? "#14171B" : nasaLight ? nasaEnamel  : (isDark ? darkBgBase : lightBgBase)
    readonly property color bgPanel:    nasaDark ? nasaDPanel : nasaLight ? nasaPanel   : (isDark ? darkBgPanel : lightBgPanel)
    readonly property color bgTrack:    nasaDark ? nasaDPanelDeep : nasaLight ? nasaPanelDeep : (isDark ? darkBgTrack : lightBgTrack)
    readonly property color bgHover:    nasaDark ? nasaDPanelHover : nasaLight ? nasaPanelHover : (isDark ? darkBgHover : lightBgHover)
    readonly property color bgDialog:   nasaDark ? "#F21D2228" : nasaLight ? "#FFFDFCF9" : (isDark ? darkBgDialog : lightBgDialog)
    readonly property color border:     nasaDark ? nasaDMetal : nasaLight ? nasaMetalLite : (isDark ? darkBorder : lightBorder)
    readonly property color textPrimary:   nasaDark ? nasaDTextPri : nasaLight ? nasaMetalDark : (isDark ? darkTextPrimary : lightTextPrimary)
    readonly property color textSecondary: nasaDark ? nasaDTextSec : nasaLight ? nasaMetalMid  : (isDark ? darkTextSecondary : lightTextSecondary)
    readonly property color textTertiary:  nasaDark ? nasaDTextTer : nasaLight ? "#8B8F93"     : (isDark ? darkTextTertiary : lightTextTertiary)
    readonly property color accent:      nasaDark ? nasaDBlue : nasaLight ? nasaBlue      : (isDark ? darkAccent : lightAccent)
    readonly property color accentHover: nasaDark ? nasaDBlueHover : nasaLight ? nasaBlueHover : (isDark ? darkAccentHover : lightAccentHover)
    readonly property color accentSoft:  nasaDark ? nasaDBlueSoft : nasaLight ? nasaBlueSoft  : (isDark ? darkAccentSoft : lightAccentSoft)
    readonly property color textOnAccent: nasaDark ? "#0E1318" : nasaLight ? "#FFFFFF"     : (isDark ? darkTextOnAccent : lightTextOnAccent)
    readonly property color danger:      nasaDark ? nasaDRed : nasaLight ? nasaRed       : (isDark ? darkDanger : lightDanger)
    readonly property color dangerHover: nasaDark ? nasaDRedHover : nasaLight ? nasaRedHover  : (isDark ? darkDangerHover : lightDangerHover)
    readonly property color dangerSoft:  nasaDark ? nasaDRedSoft : nasaLight ? nasaRedSoft   : (isDark ? darkDangerSoft : lightDangerSoft)
    readonly property color success:     nasaDark ? nasaDGreen : nasaLight ? nasaGreen     : (isDark ? darkSuccess : lightSuccess)
    // 进行中：NASA 模式下用火箭橙，与蓝图蓝形成对比且不引入粉彩
    readonly property color info:        nasaDark ? nasaDOrange : nasaLight ? nasaOrange    : (isDark ? darkInfo : lightInfo)

    // ================= NASA 结构件令牌（TitleBar / 装饰共用）=================
    readonly property color nasaChrome:     nasaDark ? nasaDChrome : nasaMetalDark
    readonly property color nasaChromeDeep: nasaDark ? nasaDChromeDeep : "#1A1C1E"
    readonly property color nasaChromeText: nasaDark ? nasaDTextPri : nasaEnamel
    readonly property color nasaChromeTextDim: nasaDark ? "#8B959D" : nasaMetalLite
    readonly property color nasaGoldAccent: nasaDark ? nasaDGold : nasaGold
    readonly property color nasaRedAccent:  nasaDark ? nasaDRed : nasaRed
    // 页面底纹（蓝图网格）——暗色下必须换成浅线，否则看不见
    readonly property color gridMinor: nasaDark ? "#1E4A82C8" : "#120B3D91"
    readonly property color gridMajor: nasaDark ? "#334A82C8" : "#260B3D91"

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
    readonly property color decoRivet:     nasaDark ? nasaDGoldDeep : nasaGoldDeep
    readonly property color decoRivetRing: nasaDark ? "#3A424A" : nasaMetalDark
    readonly property color decoSeam:      nasaDark ? nasaDMetal : nasaMetalLite
    readonly property color decoGridMajor: "#33FFFFFF"
    readonly property color decoGridMinor: "#1FFFFFFF"
    readonly property color decoHazardA:   "#1A1C1E"
    readonly property color decoHazardB:   nasaDark ? nasaDGold : nasaGold
}
