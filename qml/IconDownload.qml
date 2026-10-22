import QtQuick
import QtQuick.Shapes

// 下载 / 检查更新图标（纯 QML 矢量绘制）
//
// 沿用 IconChipTemp / IconBluetooth 的那套语言：粗线轮廓、方角、miter 转角、
// 单色描边（颜色由调用方决定）。图标优先用这套写法，不要再往 res/icon 放 PNG。
//
// 字形：一支向下的箭头 + 一个承托的托盘，语义直白（下载更新）。
// 坐标系固定为 24 x 24，通过 Scale 缩放到 size。
Item {
    id: root

    /// 描边颜色
    property color color: AppTheme.textSecondary
    /// 图标边长（正方形）
    property real size: 18
    /// 描边粗细（在 24x24 坐标系内）
    property real weightBold: 2.0

    implicitWidth: size
    implicitHeight: size

    Shape {
        width: 24
        height: 24
        transform: Scale {
            xScale: root.size / 24
            yScale: root.size / 24
        }

        // ---- 下箭头：竖杆 + 箭镞 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightBold
            capStyle: ShapePath.FlatCap
            joinStyle: ShapePath.MiterJoin
            startX: 12; startY: 3
            PathLine { x: 12; y: 14 }
            PathMove { x: 7.6; y: 9.6 }
            PathLine { x: 12; y: 14 }
            PathLine { x: 16.4; y: 9.6 }
        }

        // ---- 承托托盘：一个 U 形 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightBold
            capStyle: ShapePath.FlatCap
            joinStyle: ShapePath.MiterJoin
            startX: 5; startY: 16
            PathLine { x: 5; y: 20.5 }
            PathLine { x: 19; y: 20.5 }
            PathLine { x: 19; y: 16 }
        }
    }
}
