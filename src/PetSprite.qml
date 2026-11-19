import QtQuick

// The built-in parrot. Imported pictures replace him from Main.qml.
Item {
    id: bird

    property string mood: "idle"
    property int frame: 0
    property int direction: 1
    property real gazeX: 0
    property real gazeY: 0
    property int sink: (mood === "sit" || mood === "nap") ? 10 : 0
    property bool blink: false
    property bool eyesShut: blink || mood === "sit" || mood === "nap"

    transform: [
        Scale {
            origin.x: bird.width / 2
            origin.y: bird.height / 2
            xScale: bird.direction < 0 ? -1 : 1
        },
        Rotation {
            origin.x: bird.width / 2
            origin.y: bird.height - 8
            angle: bird.mood === "drag" ? -8
                 : (bird.mood === "preen" ? 6
                 : (bird.mood === "hop" ? -4
                 : (bird.mood === "excited" ? 3 : 0)))
        }
    ]

    Timer {
        interval: 2400
        running: !bird.eyesShut
        repeat: true
        onTriggered: bird.blink = true
    }
    Timer {
        interval: 130
        running: bird.blink
        onTriggered: bird.blink = false
    }

    Rectangle {
        x: 52
        y: 146
        width: 72
        height: 7
        radius: 3
        color: "#000000"
        opacity: 0.28
    }

    Rectangle {
        x: 12
        y: (bird.mood === "hop" || bird.mood === "excited" ? 78 : 86) + bird.sink
        width: 42
        height: 16
        radius: 8
        rotation: bird.mood === "hop" || bird.mood === "excited" ? -38 : -26
        antialiasing: true
        color: "#146b44"
    }
    Rectangle {
        x: 18
        y: (bird.mood === "hop" || bird.mood === "excited" ? 94 : 100) + bird.sink
        width: 36
        height: 13
        radius: 6
        rotation: bird.mood === "hop" || bird.mood === "excited" ? -14 : -6
        antialiasing: true
        color: "#1d8a56"
    }

    Rectangle {
        x: 58
        y: bird.mood === "walk" && bird.frame === 0 ? 128 : 138
        width: 26
        height: 8
        radius: 3
        color: "#ef8a3a"
    }
    Rectangle {
        x: 96
        y: bird.mood === "walk" && bird.frame === 1 ? 128 : 138
        width: 26
        height: 8
        radius: 3
        color: "#d87428"
    }

    Rectangle {
        x: 34
        y: 70 + bird.sink
        width: 90
        height: 66
        radius: 32
        antialiasing: true
        color: "#2f9e6b"
    }
    Rectangle {
        x: 62
        y: 96 + bird.sink
        width: 48
        height: 32
        radius: 16
        antialiasing: true
        color: "#e4ee96"
    }
    Rectangle {
        x: 46
        y: 82 + bird.sink
        width: 40
        height: 30
        radius: 14
        antialiasing: true
        rotation: bird.mood === "walk" ? (bird.frame === 0 ? -14 : 16) : -4
        color: "#1b7348"
    }

    Rectangle {
        x: 84
        y: 34 + bird.sink
        width: 58
        height: 52
        radius: 26
        antialiasing: true
        color: "#3cba78"
    }

    Rectangle {
        x: 112
        y: (bird.eyesShut ? 56 : 48) + bird.sink
        width: 16
        height: bird.eyesShut ? 3 : 16
        radius: 8
        antialiasing: true
        color: "#f7f4ea"
    }
    Rectangle {
        visible: !bird.eyesShut
        x: ((bird.mood === "talk" || bird.mood === "excited") ? 120 : 118) + Math.round(bird.gazeX * 3)
        y: 52 + bird.sink + Math.round(bird.gazeY * 3)
        width: 7
        height: 7
        radius: 4
        color: "#161814"
    }

    Rectangle {
        x: 128
        y: ((bird.mood === "talk" || bird.mood === "excited") ? 50 : 58) + bird.sink
        width: 30
        height: 13
        radius: 4
        antialiasing: true
        color: "#ef8f32"
    }
    Rectangle {
        visible: bird.mood === "talk" || bird.mood === "excited"
        x: 132
        y: 66 + bird.sink
        width: 24
        height: 9
        radius: 4
        antialiasing: true
        color: "#d56d28"
    }

    Rectangle {
        x: 96
        y: 78 + bird.sink
        width: 7
        height: 7
        radius: 4
        color: theme.accent
    }

    Rectangle {
        x: 90
        y: 16 + bird.sink
        width: 52
        height: 26
        radius: 6
        color: "#2b221c"
    }
    Rectangle {
        x: 74
        y: 36 + bird.sink
        width: 80
        height: 10
        radius: 3
        color: "#1a1411"
    }
    Rectangle {
        x: 92
        y: 30 + bird.sink
        width: 48
        height: 8
        color: theme.accent
    }
    Rectangle {
        x: 108
        y: 20 + bird.sink
        width: 9
        height: 9
        radius: 5
        color: "#f4f1ea"
    }
    Rectangle {
        x: 110
        y: 22 + bird.sink
        width: 2
        height: 2
        radius: 1
        color: "#161814"
    }
    Rectangle {
        x: 114
        y: 22 + bird.sink
        width: 2
        height: 2
        radius: 1
        color: "#161814"
    }
}
