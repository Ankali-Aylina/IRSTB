import QtQuick

// NASA 朋克装饰：数据读数下方的细刻度条。
// 等间距短竖线，每隔 5 格加长一档，模拟仪表刻度。
Item {
    id: root
    visible: AppTheme.nasa

    property int tickCount: 48
    property real tickStep: 4
    property color tickColor: AppTheme.border
    property real tickHeight: height

    Row {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: root.tickStep - 1

        Repeater {
            model: root.tickCount
            delegate: Rectangle {
                width: 1
                height: (index % 5 === 0) ? root.tickHeight : root.tickHeight * 0.55
                color: root.tickColor
            }
        }
    }
}
