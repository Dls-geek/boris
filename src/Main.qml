import QtQuick

Window {
    id: win
    width: backend.pet.windowWidth
    height: backend.pet.windowHeight
    visible: backend.revealed
    color: "transparent"
    title: "Boris"
    flags: Qt.FramelessWindowHint | Qt.NoDropShadowWindowHint

    // Fallback clear if Backend's speech timer is not running yet.
    Timer {
        id: speechFade
        interval: 4000
        repeat: false
        onTriggered: backend.personality.clearLine()
    }

    Connections {
        target: backend
        function onSpeechLineChanged() {
            if (backend.speechLine && backend.speechLine.length > 0)
                speechFade.restart()
            else
                speechFade.stop()
        }
    }

    // If talk fires without Backend.showSpeech / replyTo, still show a local line.
    Connections {
        target: backend.pet
        function onTalked() {
            if (!backend.speechLine || backend.speechLine.length === 0)
                backend.personality.replyTo("")
        }
    }

    Item {
        id: spriteHost
        x: backend.pet.insetX
        y: 0
        width: backend.pet.bodyWidth
        height: backend.pet.bodyHeight

        PetSprite {
            anchors.fill: parent
            visible: backend.avatar.kind === "parrot"
            mood: backend.pet.mood
            frame: backend.pet.frame
            direction: backend.pet.direction
            gazeX: backend.pet.gazeX
            gazeY: backend.pet.gazeY
        }

        AvatarLook {
            anchors.fill: parent
            anchors.margins: 4
            visible: backend.avatar.kind !== "parrot"
            source: backend.avatar.kind === "parrot" ? "" : backend.avatar.source
            mirror: backend.pet.direction < 0
            lookRotation: backend.pet.mood === "preen" ? 4 : (backend.pet.mood === "nap" ? -2 : 0)
            onFailed: backend.note(qsTr("The picture could not be read."))
        }

        // Short reply line — kept inside the pet window so the mask still fits.
        Rectangle {
            id: speechBubble
            visible: backend.speechLine && backend.speechLine.length > 0
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 2
            width: Math.min(parent.width - 8, speechLabel.implicitWidth + 16)
            height: speechLabel.implicitHeight + 10
            radius: 8
            color: "#e8ecf4"
            border.color: "#2a3140"
            border.width: 1
            opacity: 0.95
            z: 10

            Text {
                id: speechLabel
                anchors.centerIn: parent
                width: Math.min(spriteHost.width - 24, 140)
                text: backend.speechLine || ""
                color: "#1a1d24"
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            preventStealing: true
            cursorShape: backend.pet.mood === "drag" ? Qt.ClosedHandCursor : Qt.OpenHandCursor
            onPressed: function (mouse) {
                const point = spriteHost.mapToGlobal(mouse.x, mouse.y)
                backend.pressAt(point.x, point.y, mouse.button)
            }
            onReleased: backend.releasePointer()
        }
    }
}
