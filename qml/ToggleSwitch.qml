import QtQuick
import QtQuick.Controls

// 动画开关（替代原来在 switch-on.png / switch-off.png 之间瞬切的做法）
//
// 原做法的问题：两张 PNG 是完全不同的图（off = 空心轨道 + 左侧实心滑块，
// on = 实心轨道 + 右侧空心滑块），点击时整体瞬切，滑块会从左边"跳"到右边，
// 轨道也从空心突然变实心，没有任何过渡，观感很突兀。
//
// 本组件用 QML 直接绘制，因此可以：
//   - 滑块 x 平滑滑动（NumberAnimation + OutCubic）
//   - 轨道填充色 / 描边色渐变（ColorAnimation）
//   - 滑块颜色渐变
// 尺寸通过 trackWidth / trackHeight 可调：
//   18x10 = 与旧 PNG 视觉尺寸一致（只加动画，不改外观）
//   34x18 = 更标准的开关比例（更清晰，动画更明显）
Button {
    id: control
    focusPolicy: Qt.NoFocus
    implicitWidth: 56
    implicitHeight: 30

    property int trackWidth: 34
    property int trackHeight: 18
    property int animationDuration: 160

    // 不画外层容器：原来这里是一个 56x30 的灰底圆角框（bgTrack + 1px 描边），
    // 套在开关外面显得多余，已移除。
    // implicitWidth/Height 仍保留 56x30 —— 只是不再绘制，用于维持可点区域与布局位置不变。
    background: null

    contentItem: Item {
        implicitWidth: control.trackWidth
        implicitHeight: control.trackHeight

        Rectangle {
            id: track
            anchors.centerIn: parent
            width: control.trackWidth
            height: control.trackHeight
            radius: height / 2

            // 关：空心轨道（只描边）  开：实心轨道
            color: control.checked ? AppTheme.accent : "transparent"
            border.width: 2
            border.color: control.checked ? AppTheme.accent : AppTheme.textSecondary

            Behavior on color { ColorAnimation { duration: control.animationDuration } }
            Behavior on border.color { ColorAnimation { duration: control.animationDuration } }

            Rectangle {
                id: knob
                width: Math.max(6, track.height - 2 * track.border.width - 4)
                height: width
                radius: width / 2
                anchors.verticalCenter: parent.verticalCenter

                // 关键：滑块位置随 checked 滑动，而不是换图"跳"过去
                x: control.checked ? track.width - width - track.border.width - 2
                                   : track.border.width + 2

                // 关：滑块为中性色  开：滑块取轨道对比色
                color: control.checked ? AppTheme.textOnAccent : AppTheme.textSecondary

                Behavior on x { NumberAnimation { duration: control.animationDuration; easing.type: Easing.OutCubic } }
                Behavior on color { ColorAnimation { duration: control.animationDuration } }
            }
        }
    }
}
