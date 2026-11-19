import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

// The small launcher inside Boris's settings window.
Item {
    id: root
    width: parent ? parent.width : 400

    Column {
        width: parent.width
        spacing: 10

        Text {
            width: parent.width
            text: qsTr("Apps")
            color: theme.accent
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: qsTr("Open a page to keep Boris. These live inside this window.")
            color: "#c5cad6"
            font.pixelSize: 14
        }

        Button {
            width: parent.width
            text: qsTr("Pet")
            Material.roundedScale: theme.radius
            onClicked: backend.openPetPage()
        }

        Button {
            width: parent.width
            text: qsTr("Avatars")
            Material.roundedScale: theme.radius
            onClicked: backend.openAvatarsPage()
        }

        Button {
            width: parent.width
            text: qsTr("Settings")
            Material.roundedScale: theme.radius
            onClicked: backend.openHomePage()
        }
    }
}
