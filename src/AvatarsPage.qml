import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

// Add named looks, preview them, and switch who Boris wears.
Item {
    id: root
    width: parent ? parent.width : 400

    property string draftName: ""

    Column {
        width: parent.width
        spacing: 10

        Text {
            width: parent.width
            text: qsTr("Avatars")
            color: theme.accent
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Choose a picture or GIF, give it a name, and save it. Tap a look to wear it.")
            color: "#c5cad6"
            font.pixelSize: 14
        }

        Text {
            width: parent.width
            text: qsTr("Add a look")
            color: theme.accent
            font.pixelSize: 16
            font.bold: true
        }

        Button {
            width: parent.width
            text: qsTr("Choose a picture")
            Material.roundedScale: theme.radius
            onClicked: backend.beginNamedAvatarImport()
        }

        Item {
            width: parent.width
            height: 140

            Rectangle {
                anchors.fill: parent
                color: "#1a1d24"
                radius: theme.radius
                border.color: theme.accent
                border.width: 1

                AvatarLook {
                    anchors.fill: parent
                    anchors.margins: 12
                    visible: backend.avatarDraft.toString().length > 0
                    source: backend.avatarDraft
                }

                Text {
                    anchors.centerIn: parent
                    visible: backend.avatarDraft.toString().length === 0
                    text: qsTr("No picture chosen yet")
                    color: "#c5cad6"
                    font.pixelSize: 14
                }
            }
        }

        TextField {
            id: nameField
            width: parent.width
            placeholderText: qsTr("Name")
            text: root.draftName
            onTextChanged: root.draftName = text
            Material.roundedScale: theme.radius
            enabled: backend.avatarDraft.toString().length > 0
        }

        Row {
            width: parent.width
            spacing: 8

            Button {
                width: (parent.width - 8) / 2
                text: qsTr("Save look")
                Material.roundedScale: theme.radius
                enabled: backend.avatarDraft.toString().length > 0
                onClicked: {
                    backend.saveNamedAvatar(root.draftName)
                    root.draftName = ""
                    nameField.text = ""
                }
            }
            Button {
                width: (parent.width - 8) / 2
                text: qsTr("Clear")
                Material.roundedScale: theme.radius
                enabled: backend.avatarDraft.toString().length > 0
                onClicked: {
                    backend.clearAvatarDraft()
                    root.draftName = ""
                    nameField.text = ""
                }
            }
        }

        Text {
            width: parent.width
            text: qsTr("Your looks")
            color: theme.accent
            font.pixelSize: 16
            font.bold: true
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            visible: backend.message.length > 0
            text: backend.message
            color: "#ffb0a8"
            font.pixelSize: 14
        }

        Repeater {
            model: backend.avatar.catalog

            delegate: Rectangle {
                width: root.width
                height: 64
                radius: theme.radius
                color: modelData.active ? "#243018" : "#1a1d24"
                border.color: modelData.active ? theme.accent : "#2a2e38"
                border.width: 1

                Row {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 10

                    Item {
                        width: 48
                        height: 48

                        PetSprite {
                            anchors.fill: parent
                            visible: modelData.kind === "parrot"
                            mood: "idle"
                            frame: 0
                            direction: 1
                        }

                        AvatarLook {
                            anchors.fill: parent
                            visible: modelData.kind !== "parrot"
                            source: modelData.preview
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 48 - 88 - 20
                        spacing: 2

                        Text {
                            width: parent.width
                            text: modelData.name
                            color: "#e6e8ee"
                            font.pixelSize: 15
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: {
                                if (modelData.active)
                                    return modelData.animated ? qsTr("Wearing now · GIF") : qsTr("Wearing now")
                                return modelData.animated ? qsTr("GIF · Tap to wear") : qsTr("Tap to wear")
                            }
                            color: "#c5cad6"
                            font.pixelSize: 12
                        }
                    }

                    Button {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: modelData.id !== "parrot"
                        text: qsTr("Remove")
                        flat: true
                        Material.roundedScale: theme.radius
                        onClicked: backend.removeAvatar(modelData.id)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: modelData.id === "parrot" ? 0 : 72
                    onClicked: backend.selectAvatar(modelData.id)
                }
            }
        }

        Button {
            width: parent.width
            text: qsTr("Import frames folder")
            Material.roundedScale: theme.radius
            onClicked: backend.importFrames()
        }
    }
}
