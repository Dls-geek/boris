import QtQuick

// Shows a saved look. AnimatedImage keeps GIF frames moving; stills stay still.
Item {
    id: root

    property url source
    property bool mirror: false
    property real lookRotation: 0
    property bool playing: true

    AnimatedImage {
        id: picture
        anchors.fill: parent
        source: root.source
        fillMode: Image.PreserveAspectFit
        mirror: root.mirror
        asynchronous: true
        playing: root.playing && visible && status === Image.Ready
        rotation: root.lookRotation
        onStatusChanged: {
            if (status === Image.Error)
                root.failed()
        }
    }

    signal failed()
}
