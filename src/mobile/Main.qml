import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 420
    height: 720
    visible: true
    title: (document.fileName || "light_md") + (document.dirty ? " *" : "") + " - light_md"
    color: palette.window

    property bool editing: false

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 8

            Label {
                text: root.editing ? "Editing" : "Reading"
                font.bold: true
                Layout.fillWidth: true
            }

            ToolButton {
                text: "Open"
                onClicked: openDialog.open()
            }
            ToolButton {
                text: "Save"
                onClicked: {
                    if (document.hasPath)
                        document.save()
                    else
                        saveDialog.open()
                }
            }
            ToolButton {
                text: root.editing ? "Read" : "Edit"
                onClicked: {
                    if (!root.editing)
                        editor.text = document.text
                    root.editing = !root.editing
                }
            }
        }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: root.editing ? 1 : 0

        ScrollView {
            clip: true
            Text {
                width: Math.max(root.width - 24, 100)
                padding: 12
                wrapMode: Text.Wrap
                textFormat: Text.MarkdownText
                text: document.text
                color: palette.windowText
            }
        }

        ScrollView {
            clip: true
            TextArea {
                id: editor
                width: Math.max(root.width - 24, 100)
                padding: 12
                wrapMode: TextEdit.Wrap
                color: palette.windowText
                // Push only; load into the field when entering Edit.
                onTextChanged: {
                    if (root.editing && text !== document.text)
                        document.text = text
                }
            }
        }
    }

    footer: Label {
        padding: 8
        text: (document.dirty ? "Unsaved · " : "")
              + "Words: " + document.wordCount
              + (document.hasPath ? " · " + document.path : " · not saved")
              + (document.lastError !== "" ? " · " + document.lastError : "")
        elide: Text.ElideMiddle
    }

    FileDialog {
        id: openDialog
        title: "Open document"
        nameFilters: ["Markdown (*.md *.markdown *.txt)", "All files (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: {
            const path = selectedFile.toLocalFile()
            if (document.load(path))
                root.editing = false
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
            document.save(path)
        }
    }

    onClosing: (close) => {
        if (document.dirty) {
            close.accepted = false
            console.warn("Unsaved changes — Save before closing.")
        }
    }
}
