import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

// Pet management: who he is, what he wears, and the day-to-day controls.
Item {
    id: root
    width: parent ? parent.width : 400

    Column {
        width: parent.width
        spacing: 10

        Text {
            width: parent.width
            text: qsTr("Pet")
            color: theme.accent
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Manage Boris here — his look, size, and a few everyday controls.")
            color: "#c5cad6"
            font.pixelSize: 14
        }

        Rectangle {
            width: parent.width
            height: 150
            radius: theme.radius
            color: "#1a1d24"
            border.color: theme.accent
            border.width: 1

            Item {
                anchors.centerIn: parent
                width: backend.pet.bodyWidth
                height: backend.pet.bodyHeight
                scale: Math.min(1, Math.min(200 / Math.max(1, width), 120 / Math.max(1, height)))

                PetSprite {
                    anchors.fill: parent
                    visible: backend.avatar.kind === "parrot"
                    mood: backend.pet.mood
                    frame: backend.pet.frame
                    direction: 1
                    gazeX: backend.pet.gazeX
                    gazeY: backend.pet.gazeY
                }

                AvatarLook {
                    anchors.fill: parent
                    anchors.margins: 4
                    visible: backend.avatar.kind !== "parrot"
                    source: backend.avatar.kind === "parrot" ? "" : backend.avatar.source
                }
            }
        }

        Text {
            width: parent.width
            text: qsTr("Name")
            color: theme.accent
            font.pixelSize: 14
            font.bold: true
        }
        Text {
            width: parent.width
            text: "Boris"
            color: "#e6e8ee"
            font.pixelSize: 16
        }

        Text {
            width: parent.width
            text: qsTr("Current look")
            color: theme.accent
            font.pixelSize: 14
            font.bold: true
        }
        Text {
            width: parent.width
            text: {
                const name = backend.avatar.displayName
                return backend.avatar.animated ? (name + " · GIF") : name
            }
            color: "#e6e8ee"
            font.pixelSize: 16
        }

        Text {
            width: parent.width
            text: {
                var unused = translator.language
                return qsTr("Size: %1% — %2 × %3")
                    .arg(Number(backend.scalePercent).toLocaleString(Qt.locale(), "f", 0))
                    .arg(Number(backend.pet.bodyWidth).toLocaleString(Qt.locale(), "f", 0))
                    .arg(Number(backend.pet.bodyHeight).toLocaleString(Qt.locale(), "f", 0))
            }
            color: "#c5cad6"
            font.pixelSize: 14
        }

        Text {
            width: parent.width
            visible: backend.pet.perched
            wrapMode: Text.WordWrap
            text: qsTr("He is perched. Walk the floor when you want him to roam again.")
            color: "#c5cad6"
            font.pixelSize: 14
        }

        Text {
            width: parent.width
            text: qsTr("Switch look")
            color: theme.accent
            font.pixelSize: 16
            font.bold: true
        }

        Repeater {
            model: backend.avatar.catalog

            delegate: Button {
                width: root.width
                text: {
                    let label = (modelData.active ? "● " : "○ ") + modelData.name
                    if (modelData.animated)
                        label += " · GIF"
                    return label
                }
                Material.roundedScale: theme.radius
                onClicked: backend.selectAvatar(modelData.id)
            }
        }

        Button {
            width: parent.width
            text: qsTr("Open Avatars")
            Material.roundedScale: theme.radius
            onClicked: backend.openAvatarsPage()
        }

        Text {
            width: parent.width
            text: qsTr("Pet settings")
            color: theme.accent
            font.pixelSize: 16
            font.bold: true
        }

        Switch {
            text: qsTr("Follow the cursor")
            checked: backend.followCursor
            onToggled: backend.followCursor = checked
        }
        Switch {
            text: qsTr("Idle antics")
            checked: backend.idleAntics
            onToggled: backend.idleAntics = checked
        }
        Switch {
            text: qsTr("Sleep when idle")
            checked: backend.sleepWhenIdle
            onToggled: backend.sleepWhenIdle = checked
        }

        Button {
            width: parent.width
            text: qsTr("Walk the floor")
            Material.roundedScale: theme.radius
            onClicked: backend.walkTheFloor()
        }

        Button {
            width: parent.width
            text: qsTr("Back to Settings")
            Material.roundedScale: theme.radius
            onClicked: backend.openHomePage()
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            visible: backend.message.length > 0
            text: backend.message
            color: "#ffb0a8"
            font.pixelSize: 14
        }
    }
}
