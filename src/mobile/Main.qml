import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 420
    height: 720
    minimumWidth: 320
    minimumHeight: 480
    visible: true
    title: (document.fileName || "light_md") + (document.dirty ? " *" : "") + " - light_md"

    Material.theme: Material.System
    Material.accent: Material.BlueGrey

    property bool editing: false
    property var pendingAction: null   // function to run after Save/Discard

    function enterEdit() {
        editor.text = document.text
        editing = true
        editor.forceActiveFocus()
    }

    function enterRead() {
        editing = false
        Qt.inputMethod.hide()
    }

    function requestOpen() {
        runOrConfirm(() => openDialog.open())
    }

    function requestNew() {
        runOrConfirm(() => {
            document.clear()
            editor.text = ""
            enterEdit()
        })
    }

    function requestSave() {
        if (document.hasPath) {
            if (!document.save())
                errorBanner.show(document.lastError)
        } else {
            saveDialog.open()
        }
    }

    function runOrConfirm(action) {
        if (!document.dirty) {
            action()
            return
        }
        pendingAction = action
        unsavedDialog.open()
    }

    header: ToolBar {
        Material.elevation: 1
        leftPadding: 4
        rightPadding: 4

        RowLayout {
            anchors.fill: parent
            spacing: 2

            Label {
                text: root.editing ? "Editing" : "Reading"
                font.bold: true
                font.pixelSize: 16
                Layout.fillWidth: true
                Layout.leftMargin: 8
                elide: Label.ElideRight
            }

            ToolButton {
                text: "New"
                display: AbstractButton.TextOnly
                implicitHeight: 44
                implicitWidth: 56
                onClicked: root.requestNew()
            }
            ToolButton {
                text: "Open"
                display: AbstractButton.TextOnly
                implicitHeight: 44
                implicitWidth: 56
                onClicked: root.requestOpen()
            }
            ToolButton {
                text: "Save"
                display: AbstractButton.TextOnly
                implicitHeight: 44
                implicitWidth: 56
                onClicked: root.requestSave()
            }
            ToolButton {
                text: root.editing ? "Read" : "Edit"
                font.bold: true
                display: AbstractButton.TextOnly
                implicitHeight: 44
                implicitWidth: 64
                onClicked: root.editing ? root.enterRead() : root.enterEdit()
            }
        }
    }

    // Soft error / status strip under the toolbar.
    Rectangle {
        id: errorBanner
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: visible ? message.implicitHeight + 16 : 0
        visible: false
        color: Material.color(Material.DeepOrange, Material.Shade700)
        z: 10

        property alias text: message.text

        function show(msg) {
            if (!msg || msg === "") {
                visible = false
                return
            }
            text = msg
            visible = true
            hideTimer.restart()
        }

        Label {
            id: message
            anchors.fill: parent
            anchors.margins: 8
            wrapMode: Text.Wrap
            color: "white"
            font.pixelSize: 13
        }
        MouseArea {
            anchors.fill: parent
            onClicked: errorBanner.visible = false
        }
        Timer {
            id: hideTimer
            interval: 5000
            onTriggered: errorBanner.visible = false
        }
    }

    StackLayout {
        anchors.fill: parent
        anchors.topMargin: errorBanner.visible ? errorBanner.height : 0
        currentIndex: root.editing ? 1 : 0

        // Reader
        ScrollView {
            id: readerView
            clip: true
            contentWidth: availableWidth

            Text {
                width: readerView.availableWidth
                padding: 16
                topPadding: 20
                bottomPadding: 28
                wrapMode: Text.Wrap
                textFormat: Text.MarkdownText
                text: document.text
                color: palette.windowText
                font.pixelSize: 17
                lineHeight: 1.35
                lineHeightMode: Text.ProportionalHeight
            }
        }

        // Editor
        ScrollView {
            id: editorView
            clip: true
            contentWidth: availableWidth

            TextArea {
                id: editor
                width: editorView.availableWidth
                padding: 16
                topPadding: 20
                bottomPadding: 28
                wrapMode: TextEdit.Wrap
                color: palette.windowText
                font.pixelSize: 16
                font.family: "monospace"
                selectByMouse: true
                persistentSelection: true

                onTextChanged: {
                    if (root.editing && text !== document.text)
                        document.text = text
                }
            }
        }
    }

    footer: ToolBar {
        Material.elevation: 0
        leftPadding: 12
        rightPadding: 12
        implicitHeight: 40

        Label {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            elide: Label.ElideMiddle
            font.pixelSize: 12
            opacity: 0.8
            text: (document.dirty ? "Unsaved · " : "")
                  + "Words: " + document.wordCount
                  + (document.hasPath ? " · " + document.fileName : " · not saved")
        }
    }

    Dialog {
        id: unsavedDialog
        anchors.centerIn: Overlay.overlay
        modal: true
        title: "Save changes?"
        standardButtons: Dialog.Save | Dialog.Discard | Dialog.Cancel
        parent: Overlay.overlay

        Label {
            wrapMode: Text.Wrap
            text: "You have unsaved changes. Do you want to save them?"
        }

        onAccepted: {
            // Save button
            const next = root.pendingAction
            root.pendingAction = null
            if (document.hasPath) {
                if (document.save()) {
                    if (next) next()
                } else {
                    errorBanner.show(document.lastError)
                }
            } else {
                // Defer the pending action until after a successful Save As.
                root.pendingAction = next
                saveDialog.open()
            }
        }
        onDiscarded: {
            const next = root.pendingAction
            root.pendingAction = null
            document.setDirty(false)
            if (next) next()
        }
        onRejected: root.pendingAction = null
    }

    FileDialog {
        id: openDialog
        title: "Open document"
        nameFilters: ["Markdown (*.md *.markdown *.txt)", "All files (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            const path = selectedFile.toLocalFile()
            if (document.load(path)) {
                editor.text = document.text
                root.enterRead()
                errorBanner.visible = false
            } else {
                errorBanner.show(document.lastError)
            }
        }
    }

    FileDialog {
        id: saveDialog
        title: "Save as"
        nameFilters: ["Markdown (*.md)", "All files (*)"]
        fileMode: FileDialog.SaveFile
        defaultSuffix: "md"
        onAccepted: {
            let path = selectedFile.toLocalFile()
            if (!/\.(md|markdown|txt)$/i.test(path))
                path += ".md"
            if (document.save(path)) {
                errorBanner.visible = false
                const next = root.pendingAction
                root.pendingAction = null
                if (next) next()
            } else {
                errorBanner.show(document.lastError)
                root.pendingAction = null
            }
        }
        onRejected: root.pendingAction = null
    }

    onClosing: (close) => {
        if (!document.dirty)
            return
        close.accepted = false
        pendingAction = () => Qt.quit()
        unsavedDialog.open()
    }

    // Phone-friendly shortcuts when a keyboard is attached.
    Shortcut {
        sequences: [StandardKey.Save]
        onActivated: root.requestSave()
    }
    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: root.requestOpen()
    }
    Shortcut {
        sequences: [StandardKey.New]
        onActivated: root.requestNew()
    }
    Shortcut {
        sequence: "Esc"
        onActivated: root.enterRead()
    }
    Shortcut {
        sequence: "Ctrl+E"
        onActivated: root.editing ? root.enterRead() : root.enterEdit()
    }
}
