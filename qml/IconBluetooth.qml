import QtQuick
import QtQuick.Shapes

// 蓝牙图标（纯 QML 矢量绘制，替代原 BLE_On.png / BLE_Off.png / BLE_Error.png）
//
// 原做法是三个 PNG（同一字形、三种颜色：白 / 猩红 / 猩红+白）配 ThemeIcon 着色，
// 既多文件又踩过「带色源图再着色会变暗」的坑。QML 矢量只需一个组件，颜色直接给。
//
// 工业感的做法：四角「定位角标」（技术图纸的 registration mark）+ 粗线符文。
//   ⚠️ 不要用完整的方框 —— 试过闭合外框，36px 下会把符文挤到中间、
//      几笔描边糊成一团，反而更差。角标只占四角空白，不压缩图形本体。
//
// 坐标系固定为 24 x 24，通过 Scale 缩放到 size。
Item {
    id: root

    /// 描边颜色（跟随连接状态：已连接绿 / 连接中天蓝 / 失败信号红 / 断开金属灰）
    property color color: AppTheme.textSecondary
    /// 图标边长（正方形）
    property real size: 36
    /// 符文主体线宽（在 24x24 坐标系内）
    /// ⚠️ 不要超过 1.6 —— 符文本身只有约 15 单位宽，描边再粗就会把笔画间隙堵死，
    ///    渲染出来是一团实心三角而不是蓝牙符文
    property real weightBold: 1.55
    /// 角标发丝线宽
    property real weightThin: 0.7
    /// 是否显示四角定位角标
    property bool brackets: true

    implicitWidth: size
    implicitHeight: size

    // ---- 四角定位角标（技术图纸感，只占角落空白）----
    Shape {
        width: 24
        height: 24
        visible: root.brackets
        transform: Scale {
            xScale: root.size / 24
            yScale: root.size / 24
        }

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightThin
            capStyle: ShapePath.FlatCap
            joinStyle: ShapePath.MiterJoin
            startX: 1.3; startY: 5.9
            PathLine { x: 1.3; y: 1.3 }
            PathLine { x: 5.9; y: 1.3 }
            PathMove { x: 18.1; y: 1.3 }
            PathLine { x: 22.7; y: 1.3 }
            PathLine { x: 22.7; y: 5.9 }
            PathMove { x: 22.7; y: 18.1 }
            PathLine { x: 22.7; y: 22.7 }
            PathLine { x: 18.1; y: 22.7 }
            PathMove { x: 5.9; y: 22.7 }
            PathLine { x: 1.3; y: 22.7 }
            PathLine { x: 1.3; y: 18.1 }
        }
    }

    // ---- 符文主体：竖线 + 右侧上下两个尖角 + 两条自左穿过的斜线 ----
    Shape {
        width: 24
        height: 24
        transform: Scale {
            xScale: root.size / 24
            yScale: root.size / 24
        }

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightBold
            capStyle: ShapePath.FlatCap
            joinStyle: ShapePath.MiterJoin
            startX: 12; startY: 3.0
            PathLine { x: 12; y: 21.0 }        // 竖线
            PathLine { x: 19.5; y: 16.5 }      // 左下斜向上到右
            PathLine { x: 12; y: 12.0 }        // 回到中线
            PathLine { x: 19.5; y: 7.5 }       // 右上斜
            PathLine { x: 12; y: 3.0 }         // 回到顶部
        }

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.color
            strokeWidth: root.weightBold
            capStyle: ShapePath.FlatCap
            startX: 4.5; startY: 7.5
            PathLine { x: 19.5; y: 16.5 }
            PathMove { x: 4.5; y: 16.5 }
            PathLine { x: 19.5; y: 7.5 }
        }
    }
}
