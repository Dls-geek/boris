import QtQuick

Window {
    id: win
    width: backend.pet.windowWidth
    height: backend.pet.windowHeight
    visible: backend.revealed
    color: "transparent"
    title: "Boris"
    flags: Qt.FramelessWindowHint | Qt.NoDropShadowWindowHint

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
