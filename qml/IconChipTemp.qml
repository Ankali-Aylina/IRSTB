import QtQuick
import QtQuick.Shapes

// CPU / GPU 温度图标（纯 QML 矢量绘制，替代原 cpuTemp.png / gpuTemp.png）
//
// 为什么用 QML 画而不是继续用 PNG：
//   1. PNG 走 ThemeIcon 时靠 MultiEffect「目标色 × 源图亮度」着色，源图必须是纯白，
//      一旦源图带上颜色就会越描越暗（v4.4.0.0 之前的暗红褐色 bug 就是这么来的）。
//      QML 矢量直接设 strokeColor，不存在这个问题。
//   2. 任意尺寸都清晰，且在 NASA 朋克主题要求「线性、几何、单色、不要拟物」时不需要新素材。
//   3. CPU 与 GPU 共用同一套芯片几何，只换内部标记 —— 风格天然一致。
//
// 坐标系固定为 28 x 25（与旧 PNG 的宽高比一致），通过 Scale 缩放到 size。
Item {
    id: root

    /// 描边颜色（跟随主题状态，如报警时用 AppTheme.danger）
    property color color: AppTheme.textSecondary
    /// "cpu" = 同心方格（芯片核心）；"gpu" = 菱形（显卡核心）
    property string kind: "cpu"
    /// 宽度；高度按 25/28 自动换算
    property real size: 44
    /// 描边粗细（在 28x25 坐标系内）
    property real stroke: 1.35

    implicitWidth: size
    implicitHeight: size * 25 / 28

    Shape {
        width: 28
        height: 25
        transform: Scale {
            xScale: root.size / 28
            yScale: root.size / 28
        }

        // ---- 芯片外框 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.stroke
            joinStyle: ShapePath.MiterJoin
            startX: 3; startY: 6
            PathLine { x: 16; y: 6 }
            PathLine { x: 16; y: 19 }
            PathLine { x: 3; y: 19 }
            PathLine { x: 3; y: 6 }
        }

        // ---- 芯片引脚（上 3 / 下 3 / 左 2 / 右 2）----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.stroke
            capStyle: ShapePath.FlatCap
            startX: 5.5; startY: 3
            PathLine { x: 5.5; y: 6 }
            PathMove { x: 9.5; y: 3 }
            PathLine { x: 9.5; y: 6 }
            PathMove { x: 13.5; y: 3 }
            PathLine { x: 13.5; y: 6 }

            PathMove { x: 5.5; y: 19 }
            PathLine { x: 5.5; y: 22 }
            PathMove { x: 9.5; y: 19 }
            PathLine { x: 9.5; y: 22 }
            PathMove { x: 13.5; y: 19 }
            PathLine { x: 13.5; y: 22 }

            PathMove { x: 0.8; y: 9.5 }
            PathLine { x: 3; y: 9.5 }
            PathMove { x: 0.8; y: 15.5 }
            PathLine { x: 3; y: 15.5 }

            PathMove { x: 16; y: 9.5 }
            PathLine { x: 18.2; y: 9.5 }
            PathMove { x: 16; y: 15.5 }
            PathLine { x: 18.2; y: 15.5 }
        }

        // ---- 芯片内部标记：CPU=同心方格，GPU=菱形 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.stroke
            joinStyle: ShapePath.MiterJoin
            startX: root.kind === "gpu" ? 9.5 : 7.5
            startY: root.kind === "gpu" ? 10.3 : 10.5
            PathLine { x: root.kind === "gpu" ? 11.7 : 11.5; y: root.kind === "gpu" ? 12.5 : 10.5 }
            PathLine { x: root.kind === "gpu" ? 9.5 : 11.5; y: root.kind === "gpu" ? 14.7 : 14.5 }
            PathLine { x: root.kind === "gpu" ? 7.3 : 7.5; y: root.kind === "gpu" ? 12.5 : 14.5 }
            PathLine { x: root.kind === "gpu" ? 9.5 : 7.5; y: root.kind === "gpu" ? 10.3 : 10.5 }
        }

        // ---- 温度计：管身 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.stroke * 1.8
            capStyle: ShapePath.RoundCap
            startX: 21.6; startY: 2.4
            PathLine { x: 21.6; y: 16.4 }
        }

        // ---- 温度计：球泡（两个半圆拼成）----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.stroke
            startX: 18.4; startY: 19.4
            PathArc { x: 24.8; y: 19.4; radiusX: 3.2; radiusY: 3.2; direction: PathArc.Clockwise }
            PathArc { x: 18.4; y: 19.4; radiusX: 3.2; radiusY: 3.2; direction: PathArc.Clockwise }
        }

        // ---- 温度计：刻度 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.stroke
            capStyle: ShapePath.FlatCap
            startX: 24.2; startY: 5.5
            PathLine { x: 26.6; y: 5.5 }
            PathMove { x: 24.2; y: 9.5 }
            PathLine { x: 26.6; y: 9.5 }
            PathMove { x: 24.2; y: 13.5 }
            PathLine { x: 26.6; y: 13.5 }
        }
    }
}
