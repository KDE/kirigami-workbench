import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs

Item {
    id: root
    implicitWidth: 400
    implicitHeight: 260

    ColumnLayout {
        anchors { fill: parent; margins: Kirigami.Units.largeSpacing }
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            id: msg
            Layout.fillWidth: true
            visible: text.length > 0
        }

        Kirigami.Heading { text: "PromptDialog variants"; level: 3 }

        RowLayout {
            Controls.Button { text: "Confirm";     onClicked: confirmDlg.open() }
            Controls.Button { text: "Information"; onClicked: infoDlg.open() }
            Controls.Button { text: "Warning";     onClicked: warnDlg.open() }
            Controls.Button { text: "Error";       onClicked: errDlg.open() }
        }

        Controls.Button { text: "With input"; onClicked: inputDlg.open() }

        Item { Layout.fillHeight: true }
    }

    KirigamiDialogs.PromptDialog {
        id: confirmDlg
        parent: root
        title: "Reset Settings?"
        subtitle: "All stored settings will be deleted and defaults restored."
        standardButtons: Controls.DialogButtonBox.Ok | Controls.DialogButtonBox.Cancel
        onAccepted: { msg.type = Kirigami.MessageType.Positive;     msg.text = "Settings reset confirmed" }
        onRejected: { msg.type = Kirigami.MessageType.Information;   msg.text = "Reset cancelled" }
    }

    KirigamiDialogs.PromptDialog {
        id: infoDlg
        parent: root
        title: "Update Available"
        subtitle: "Version 2.1 is ready. Restart the app to apply it."
        dialogType: KirigamiDialogs.PromptDialog.Information
        standardButtons: Controls.DialogButtonBox.Ok
        onAccepted: { msg.type = Kirigami.MessageType.Information; msg.text = "Update acknowledged" }
    }

    KirigamiDialogs.PromptDialog {
        id: warnDlg
        parent: root
        title: "Low Disk Space"
        subtitle: "Only 500 MB remaining. Consider freeing up files."
        dialogType: KirigamiDialogs.PromptDialog.Warning
        standardButtons: Controls.DialogButtonBox.Ok
        onAccepted: { msg.type = Kirigami.MessageType.Warning; msg.text = "Warning dismissed" }
    }

    KirigamiDialogs.PromptDialog {
        id: errDlg
        parent: root
        title: "File Not Found"
        subtitle: "The file could not be located. It may have been moved or deleted."
        dialogType: KirigamiDialogs.PromptDialog.Error
        standardButtons: Controls.DialogButtonBox.Ok
        onAccepted: { msg.type = Kirigami.MessageType.Error; msg.text = "Error dismissed" }
    }

    KirigamiDialogs.PromptDialog {
        id: inputDlg
        parent: root
        title: "New Folder"
        standardButtons: Controls.DialogButtonBox.NoButton
        customFooterActions: [
            Kirigami.Action {
                text: "Create"
                icon.name: "folder-new-symbolic"
                onTriggered: {
                    msg.type = Kirigami.MessageType.Positive
                    msg.text = folderField.text.length > 0 ? "Created: " + folderField.text : "No name entered"
                    folderField.clear(); inputDlg.close()
                }
            },
            Kirigami.Action {
                text: "Cancel"
                icon.name: "dialog-cancel-symbolic"
                onTriggered: { folderField.clear(); inputDlg.close() }
            }
        ]
        Controls.TextField {
            id: folderField
            placeholderText: "Folder name\u2026"
            width: Kirigami.Units.gridUnit * 16
        }
    }
}
