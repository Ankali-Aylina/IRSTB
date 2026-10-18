import QtQuick
import QtQuick.Shapes

// 蓝牙图标（纯 QML 矢量绘制，替代原 BLE_On.png / BLE_Off.png / BLE_Error.png）
//
// 原做法是三个 PNG（同一字形、三种颜色：白 / 猩红 / 猩红+白）配 ThemeIcon 着色，
// 既多文件又踩过「带色源图再着色会变暗」的坑。QML 矢量只需一个组件，颜色直接给。
//
// 字形为经典蓝牙符文：中央竖线 + 右侧上下两个尖角 + 两条自左穿过的斜线。
// 坐标系固定为 24 x 24，通过 Scale 缩放到 size。
Item {
    id: root

    /// 描边颜色（跟随连接状态：已连接绿 / 连接中天蓝 / 失败信号红 / 断开金属灰）
    property color color: AppTheme.textSecondary
    /// 图标边长（正方形）
    property real size: 36
    /// 描边粗细（在 24x24 坐标系内）
    property real stroke: 2.0

    implicitWidth: size
    implicitHeight: size

    Shape {
        width: 24
        height: 24
        transform: Scale {
            xScale: root.size / 24
            yScale: root.size / 24
        }

        // ---- 字形主轮廓：竖线 + 上下两个右侧尖角 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.stroke
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            startX: 12; startY: 2.6
            PathLine { x: 12; y: 21.4 }        // 竖线向下
            PathLine { x: 19; y: 16.25 }       // 左下斜向上到右
            PathLine { x: 12; y: 12.0 }        // 回到中线
            PathLine { x: 19; y: 8.25 }        // 右上斜
            PathLine { x: 12; y: 2.6 }         // 回到顶部
        }

        // ---- 两条自左穿过的斜线 ----
        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.stroke
            capStyle: ShapePath.RoundCap
            startX: 5; startY: 8.25
            PathLine { x: 19; y: 16.25 }
            PathMove { x: 5; y: 16.25 }
            PathLine { x: 19; y: 8.25 }
        }
    }
}
