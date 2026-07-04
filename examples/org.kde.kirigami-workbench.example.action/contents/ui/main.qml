import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Item {
    implicitWidth: 460
    implicitHeight: 420

    ColumnLayout {
        anchors { fill: parent; margins: Kirigami.Units.largeSpacing }
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            id: msg
            Layout.fillWidth: true
            visible: text.length > 0
        }

        Kirigami.Heading { text: "Action in a toolbar"; level: 3 }

        Kirigami.ActionToolBar {
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    text: "Add";    icon.name: "list-add-symbolic"
                    onTriggered: msg.text = "Add triggered"
                },
                Kirigami.Action {
                    text: "Edit";   icon.name: "document-edit-symbolic"
                    onTriggered: msg.text = "Edit triggered"
                },
                Kirigami.Action {
                    text: "Delete"; icon.name: "edit-delete-symbolic"
                    onTriggered: msg.text = "Delete triggered"
                },
                Kirigami.Action { separator: true },
                Kirigami.Action {
                    text: "Toggle"; icon.name: "view-visible-symbolic"
                    checkable: true
                    onToggled: msg.text = checked ? "Toggle ON" : "Toggle OFF"
                }
            ]
        }

        Kirigami.Heading { text: "ContextualHelpButton"; level: 3 }

        RowLayout {
            Controls.Label { text: "Some field:" }
            Controls.TextField { placeholderText: "Enter value\u2026" }
            Kirigami.ContextualHelpButton {
                toolTipText: "Click for guidance on this field"
            }
        }

        Kirigami.Heading { text: "Nested actions (submenu)"; level: 3 }

        Kirigami.ActionToolBar {
            Layout.fillWidth: true
            actions: [
                Kirigami.Action {
                    text: "File"
                    Kirigami.Action { text: "New";  icon.name: "document-new-symbolic";  onTriggered: msg.text = "File → New" }
                    Kirigami.Action { text: "Open"; icon.name: "document-open-symbolic"; onTriggered: msg.text = "File → Open" }
                    Kirigami.Action { text: "Save"; icon.name: "document-save-symbolic"; onTriggered: msg.text = "File → Save" }
                }
            ]
        }

        Item { Layout.fillHeight: true }
    }
}
