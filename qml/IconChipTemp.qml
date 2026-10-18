import QtQuick
import QtQuick.Shapes

// CPU / GPU 温度图标（纯 QML 矢量绘制，替代原 cpuTemp.png / gpuTemp.png）
//
// 为什么用 QML 画而不是 PNG：
//   1. PNG 走 ThemeIcon 时靠 MultiEffect「目标色 × 源图亮度」着色，源图必须是纯白，
//      一旦带色就会越描越暗（v4.4.0.0 之前那个暗红褐色 bug 就是这么来的）。
//      QML 矢量直接设 strokeColor，不存在这个问题。
//   2. 任意尺寸都清晰，且换主题时可同时换线宽。
//   3. CPU 与 GPU 共用同一套芯片几何，只换内部标记 —— 风格天然一致。
//
// 工业感来自技术插图的几条惯例：
//   · 双线重：粗线画轮廓（封装 / 焊盘 / 温度计），细发丝线画内部细节（芯片核心、刻度）
//   · 封装左上角做 45° 倒角 + Pin-1 实心标记 —— 真实 IC 数据手册的画法
//   · 引脚画成短粗焊盘而不是细线
//   · 温度计做成「分级刻度」：长短交替，而不是等长线
//
// 坐标系固定为 28 x 25（与旧 PNG 的宽高比一致），通过 Scale 缩放到 size。
Item {
    id: root

    /// 描边颜色（跟随主题状态，如报警时用 AppTheme.danger）
    property color color: AppTheme.textSecondary
    /// "cpu" = 芯片核心为同心方格；"gpu" = 芯片核心为菱形
    property string kind: "cpu"
    /// 宽度；高度按 25/28 自动换算
    property real size: 44
    /// 轮廓线宽（在 28x25 坐标系内）
    property real weightBold: 1.5
    /// 细部线宽
    property real weightThin: 0.85

    implicitWidth: size
    implicitHeight: size * 25 / 28

    Shape {
        width: 28
        height: 25
        transform: Scale {
            xScale: root.size / 28
            yScale: root.size / 28
        }

        // ---- 芯片封装：左上角 45° 倒角（Pin-1 方位）----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightBold
            joinStyle: ShapePath.MiterJoin
            startX: 5.6; startY: 5.4
            PathLine { x: 15.0; y: 5.4 }
            PathLine { x: 15.0; y: 17.6 }
            PathLine { x: 3.6; y: 17.6 }
            PathLine { x: 3.6; y: 7.4 }
            PathLine { x: 5.6; y: 5.4 }
        }

        // ---- 焊盘：四边各 3 个短粗块（比细线更像真实引脚焊盘）----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: 1.25
            capStyle: ShapePath.FlatCap
            // 上
            startX: 6.6; startY: 3.9
            PathLine { x: 6.6; y: 5.4 }
            PathMove { x: 9.3; y: 3.9 }
            PathLine { x: 9.3; y: 5.4 }
            PathMove { x: 12.0; y: 3.9 }
            PathLine { x: 12.0; y: 5.4 }
            // 下
            PathMove { x: 6.6; y: 17.6 }
            PathLine { x: 6.6; y: 19.1 }
            PathMove { x: 9.3; y: 17.6 }
            PathLine { x: 9.3; y: 19.1 }
            PathMove { x: 12.0; y: 17.6 }
            PathLine { x: 12.0; y: 19.1 }
            // 左
            PathMove { x: 2.2; y: 8.3 }
            PathLine { x: 3.6; y: 8.3 }
            PathMove { x: 2.2; y: 11.5 }
            PathLine { x: 3.6; y: 11.5 }
            PathMove { x: 2.2; y: 14.7 }
            PathLine { x: 3.6; y: 14.7 }
            // 右
            PathMove { x: 15.0; y: 8.3 }
            PathLine { x: 16.4; y: 8.3 }
            PathMove { x: 15.0; y: 11.5 }
            PathLine { x: 16.4; y: 11.5 }
            PathMove { x: 15.0; y: 14.7 }
            PathLine { x: 16.4; y: 14.7 }
        }

        // ---- Pin-1 标记：倒角内侧的实心小方块 ----
        ShapePath {
            fillColor: root.color
            strokeColor: "transparent"
            strokeWidth: 0
            startX: 4.7; startY: 6.4
            PathLine { x: 5.5; y: 6.4 }
            PathLine { x: 5.5; y: 7.2 }
            PathLine { x: 4.7; y: 7.2 }
            PathLine { x: 4.7; y: 6.4 }
        }

        // ---- 芯片核心（die）：发丝线方框 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightThin
            joinStyle: ShapePath.MiterJoin
            startX: 7.4; startY: 9.4
            PathLine { x: 11.2; y: 9.4 }
            PathLine { x: 11.2; y: 13.4 }
            PathLine { x: 7.4; y: 13.4 }
            PathLine { x: 7.4; y: 9.4 }
        }

        // ---- 核心内部标记：CPU=同心方格，GPU=菱形 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightThin
            joinStyle: ShapePath.MiterJoin
            startX: root.kind === "gpu" ? 9.3 : 8.4
            startY: root.kind === "gpu" ? 10.1 : 10.4
            PathLine { x: root.kind === "gpu" ? 11.0 : 10.2; y: root.kind === "gpu" ? 11.4 : 10.4 }
            PathLine { x: root.kind === "gpu" ? 9.3 : 10.2; y: root.kind === "gpu" ? 12.7 : 12.4 }
            PathLine { x: root.kind === "gpu" ? 7.6 : 8.4; y: root.kind === "gpu" ? 11.4 : 12.4 }
            PathLine { x: root.kind === "gpu" ? 9.3 : 8.4; y: root.kind === "gpu" ? 10.1 : 10.4 }
        }

        // ---- 温度计管身（粗线，构成主轮廓）----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightBold
            capStyle: ShapePath.FlatCap
            startX: 22.6; startY: 1.1
            PathLine { x: 22.6; y: 16.2 }
        }

        // ---- 温度计球泡（粗线描边，与管身同属主轮廓）----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightBold
            startX: 19.7; startY: 18.5
            PathArc { x: 25.5; y: 18.5; radiusX: 2.9; radiusY: 2.9; direction: PathArc.Clockwise }
            PathArc { x: 19.7; y: 18.5; radiusX: 2.9; radiusY: 2.9; direction: PathArc.Clockwise }
        }

        // ---- 球泡内的液面（实心，细部）----
        ShapePath {
            fillColor: root.color
            strokeColor: "transparent"
            strokeWidth: 0
            startX: 21.45; startY: 18.5
            PathArc { x: 23.75; y: 18.5; radiusX: 1.15; radiusY: 1.15; direction: PathArc.Clockwise }
            PathArc { x: 21.45; y: 18.5; radiusX: 1.15; radiusY: 1.15; direction: PathArc.Clockwise }
        }

        // ---- 温度计刻度：长短交替（技术插图的刻度惯例）----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightThin
            capStyle: ShapePath.FlatCap
            startX: 25.2; startY: 3.7
            PathLine { x: 27.2; y: 3.7 }
            PathMove { x: 25.2; y: 6.7 }
            PathLine { x: 26.2; y: 6.7 }
            PathMove { x: 25.2; y: 9.7 }
            PathLine { x: 27.2; y: 9.7 }
            PathMove { x: 25.2; y: 12.7 }
            PathLine { x: 26.2; y: 12.7 }
        }
    }
}
