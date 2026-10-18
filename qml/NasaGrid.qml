import QtQuick

// NASA 朋克装饰：双尺度蓝图网格（细网格 + 每 5 格一条粗网格）。
// 用于 NASA 蓝底的「深色结构区块」，模拟 1960-80 年代蓝图图纸。
Canvas {
    id: root
    visible: AppTheme.nasa

    property real minorStep: 8
    property real majorStep: 40
    property color minorColor: AppTheme.decoGridMinor
    property color majorColor: AppTheme.decoGridMajor

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)

        ctx.lineWidth = 1
        ctx.strokeStyle = minorColor
        ctx.beginPath()
        for (var x = minorStep; x < width; x += minorStep) {
            if (Math.abs(x % majorStep) < 0.01)
                continue
            ctx.moveTo(x + 0.5, 0)
            ctx.lineTo(x + 0.5, height)
        }
        for (var y = minorStep; y < height; y += minorStep) {
            if (Math.abs(y % majorStep) < 0.01)
                continue
            ctx.moveTo(0, y + 0.5)
            ctx.lineTo(width, y + 0.5)
        }
        ctx.stroke()

        ctx.strokeStyle = majorColor
        ctx.beginPath()
        for (var xm = majorStep; xm < width; xm += majorStep) {
            ctx.moveTo(xm + 0.5, 0)
            ctx.lineTo(xm + 0.5, height)
        }
        for (var ym = majorStep; ym < height; ym += majorStep) {
            ctx.moveTo(0, ym + 0.5)
            ctx.lineTo(width, ym + 0.5)
        }
        ctx.stroke()
    }

    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onVisibleChanged: if (visible) requestPaint()
    Component.onCompleted: requestPaint()
}
