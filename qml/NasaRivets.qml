import QtQuick

// NASA 朋克装饰：面板四角的小铆钉。
// 只在 NASA 模式下可见；用法是把它作为面板 Rectangle 的最后一个子元素并 anchors.fill。
// 尺寸刻意做得够大（7px + 1px 描边），否则缩到 5px 时只像个小点，读不出「螺钉」的意思。
Item {
    id: root
    visible: AppTheme.nasa

    property real rivetInset: 8
    property real rivetSize: 7

    Repeater {
        model: 4
        delegate: Rectangle {
            width: root.rivetSize
            height: root.rivetSize
            radius: width / 2
            color: AppTheme.decoRivet
            border.width: 1
            border.color: AppTheme.nasaMetalDark
            x: (index % 2 === 0) ? root.rivetInset : root.width - root.rivetInset - width
            y: (index < 2) ? root.rivetInset : root.height - root.rivetInset - height
        }
    }
}
