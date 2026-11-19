import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

// A real window for keeping Boris, separate from the little overlay he walks in.
Window {
    id: settings
    width: backend.settingsWidth
    height: backend.settingsHeight
    visible: backend.settingsOpen
    color: "#12141a"
    title: "Boris"
    flags: Qt.FramelessWindowHint | Qt.NoDropShadowWindowHint

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool shortcutBanner: false

    readonly property string pageTitle: {
        if (backend.settingsPage === "menu")
            return qsTr("Apps")
        if (backend.settingsPage === "avatars")
            return qsTr("Avatars")
        if (backend.settingsPage === "pet")
            return qsTr("Pet")
        return "Boris"
    }

    Shortcut {
        sequences: ["?"]
        context: Qt.WindowShortcut
        onActivated: settings.shortcutBanner = !settings.shortcutBanner
    }
    Shortcut {
        sequence: "Q"
        context: Qt.WindowShortcut
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: "Escape"
        context: Qt.WindowShortcut
        onActivated: {
            if (backend.settingsPage === "home")
                backend.closeSettings()
            else if (backend.settingsPage === "menu")
                backend.openHomePage()
            else
                backend.openAppsMenu()
        }
    }
    Shortcut {
        sequence: "Space"
        context: Qt.WindowShortcut
        onActivated: backend.pet.poke()
    }

    Rectangle {
        anchors.fill: parent
        color: "#12141a"
        border.color: theme.accent
        border.width: 2
        radius: theme.radius

        Item {
            id: page
            anchors.fill: parent
            anchors.margins: 2
            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true

            Rectangle {
                id: titleBar
                anchors.left: parent.left
                anchors.right: parent.right
                height: 52
                color: "#1a1d24"

                Button {
                    id: backButton
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    visible: backend.settingsPage !== "home"
                    text: qsTr("Back")
                    flat: true
                    Material.roundedScale: theme.radius
                    onClicked: {
                        if (backend.settingsPage === "menu")
                            backend.openHomePage()
                        else
                            backend.openAppsMenu()
                    }
                }

                Text {
                    anchors.left: backButton.visible ? backButton.right : parent.left
                    anchors.leftMargin: backButton.visible ? 4 : 16
                    anchors.verticalCenter: parent.verticalCenter
                    text: settings.pageTitle
                    color: theme.accent
                    font.pixelSize: 20
                    font.bold: true
                }

                Button {
                    id: appsButton
                    anchors.right: closeButton.left
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    visible: backend.settingsPage === "home"
                    text: qsTr("Apps")
                    flat: true
                    Material.roundedScale: theme.radius
                    onClicked: backend.openAppsMenu()
                }

                Button {
                    id: closeButton
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Close")
                    flat: true
                    Material.roundedScale: theme.radius
                    onClicked: backend.closeSettings()
                }

                MouseArea {
                    anchors.left: backButton.visible ? backButton.right : parent.left
                    anchors.right: appsButton.visible ? appsButton.left : closeButton.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    cursorShape: Qt.SizeAllCursor
                    onPressed: function (mouse) {
                        const point = titleBar.mapToGlobal(mouse.x, mouse.y)
                        backend.beginSettingsDrag(point.x, point.y)
                    }
                    onReleased: backend.endSettingsDrag()
                }
            }

            Flickable {
                id: scroller
                anchors.top: titleBar.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 16
                contentWidth: width
                contentHeight: column.implicitHeight
                clip: true
                flickableDirection: Flickable.VerticalFlick
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: column
                    width: scroller.width
                    spacing: 8

                    Loader {
                        id: pageLoader
                        width: parent.width
                        sourceComponent: {
                            if (backend.settingsPage === "menu")
                                return appsMenuPage
                            if (backend.settingsPage === "avatars")
                                return avatarsPage
                            if (backend.settingsPage === "pet")
                                return petPage
                            return homePage
                        }
                    }
                }
            }
        }
    }

    Component {
        id: appsMenuPage
        AppsMenu {}
    }
    Component {
        id: avatarsPage
        AvatarsPage {}
    }
    Component {
        id: petPage
        PetPage {}
    }
    Component {
        id: homePage
        Column {
            width: pageLoader.width
            spacing: 8

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Click him for a hop. Drag him to move him. Right-click to open settings.")
                color: "#e6e8ee"
                font.pixelSize: 14
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("Drag the title to move this window.")
                color: "#c5cad6"
                font.pixelSize: 13
            }

            Text {
                width: parent.width
                text: qsTr("Size")
                color: theme.accent
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("The size changes him on the desktop right away.")
                color: "#c5cad6"
                font.pixelSize: 14
            }

            Row {
                width: parent.width
                spacing: 12

                // Label width is fixed so the slider track does not resize
                // mid-drag (that remaps the handle and snaps size backward).
                Slider {
                    id: sizeSlider
                    width: parent.width - sizeLabel.width - 12
                    from: 40
                    to: 200
                    stepSize: 10
                    snapMode: Slider.SnapAlways
                    Component.onCompleted: value = backend.scalePercent
                    onMoved: backend.setScalePercent(Math.round(value / 10) * 10)
                }

                Text {
                    id: sizeLabel
                    width: 56
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    text: {
                        var unused = translator.language
                        return qsTr("%1%").arg(Number(backend.scalePercent).toLocaleString(Qt.locale(), "f", 0))
                    }
                    color: "#e6e8ee"
                    font.pixelSize: 16
                }

                Connections {
                    target: backend
                    function onScalePercentChanged() {
                        if (!sizeSlider.pressed)
                            sizeSlider.value = backend.scalePercent
                    }
                }
            }

            Text {
                width: parent.width
                text: {
                    var unused = translator.language
                    return qsTr("%1 × %2").arg(Number(backend.pet.bodyWidth).toLocaleString(Qt.locale(), "f", 0))
                                           .arg(Number(backend.pet.bodyHeight).toLocaleString(Qt.locale(), "f", 0))
                }
                color: "#e6e8ee"
                font.pixelSize: 14
            }

            Button {
                text: qsTr("Default size")
                Material.roundedScale: theme.radius
                onClicked: backend.setScalePercent(100)
            }

            Item {
                width: parent.width
                height: 120
                clip: true

                Item {
                    width: backend.pet.bodyWidth
                    height: backend.pet.bodyHeight
                    anchors.centerIn: parent
                    scale: Math.min(1, Math.min(220 / Math.max(1, width), 150 / Math.max(1, height)))
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
                text: qsTr("Attention")
                color: theme.accent
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: qsTr("He glances at the cursor, amuses himself, and naps when left alone.")
                color: "#c5cad6"
                font.pixelSize: 14
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

            Text {
                width: parent.width
                text: qsTr("Look")
                color: theme.accent
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                visible: backend.pet.perched
                text: qsTr("He is perched. Walk the floor when you want him to roam again.")
                color: "#c5cad6"
                font.pixelSize: 14
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                visible: backend.message.length > 0
                text: backend.message
                color: "#ffb0a8"
                font.pixelSize: 14
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                visible: settings.shortcutBanner
                text: qsTr("Space hops. Q quits. Escape closes this window.")
                color: "#c5cad6"
                font.pixelSize: 14
            }

            Button {
                width: parent.width
                text: qsTr("Open Avatars")
                Material.roundedScale: theme.radius
                onClicked: backend.openAvatarsPage()
            }
            Button {
                width: parent.width
                text: qsTr("Open Pet")
                Material.roundedScale: theme.radius
                onClicked: backend.openPetPage()
            }
            Button {
                width: parent.width
                text: qsTr("Use the parrot")
                Material.roundedScale: theme.radius
                onClicked: backend.useParrot()
            }
            Button {
                width: parent.width
                text: qsTr("Walk the floor")
                Material.roundedScale: theme.radius
                onClicked: backend.walkTheFloor()
            }
            Button {
                width: parent.width
                text: qsTr("Quit")
                Material.roundedScale: theme.radius
                onClicked: Qt.quit()
            }
        }
    }
}
