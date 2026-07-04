import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Item {
    implicitWidth: 460
    implicitHeight: 600

    ColumnLayout {
        anchors { fill: parent; margins: Kirigami.Units.largeSpacing }
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            id: msg
            Layout.fillWidth: true
            visible: text.length > 0
        }

        Kirigami.Heading { text: "Button variants"; level: 3 }

        RowLayout {
            Controls.Button { text: "Default";  onClicked: msg.text = "Default clicked" }
            Controls.Button { text: "Primary";  highlighted: true; onClicked: msg.text = "Primary clicked" }
            Controls.Button { text: "Flat";     flat: true;        onClicked: msg.text = "Flat clicked" }
            Controls.Button { text: "Disabled"; enabled: false }
        }

        Kirigami.Heading { text: "ToolButton display modes"; level: 3 }

        RowLayout {
            Controls.ToolButton {
                icon.name: "list-add-symbolic"
                text: "Add"
                display: Controls.AbstractButton.TextBesideIcon
                onClicked: msg.text = "TextBesideIcon"
            }
            Controls.ToolButton {
                icon.name: "document-save-symbolic"
                text: "Save"
                display: Controls.AbstractButton.TextUnderIcon
                onClicked: msg.text = "TextUnderIcon"
            }
        }

        Kirigami.Heading { text: "Button with icon"; level: 3 }

        RowLayout {
            Controls.Button {
                icon.name: "list-add-symbolic"
                text: "New File"
                highlighted: true
                onClicked: msg.text = "New File clicked"
            }
            Controls.Button {
                icon.name: "edit-delete-symbolic"
                text: "Delete"
                onClicked: msg.text = "Delete clicked"
            }
        }

        Kirigami.Heading { text: "Theme colours"; level: 3 }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: [
                    { label: "Highlight",  color: Kirigami.Theme.highlightColor,       text: Kirigami.Theme.highlightedTextColor },
                    { label: "Positive",   color: Kirigami.Theme.positiveTextColor,     text: "white" },
                    { label: "Neutral",    color: Kirigami.Theme.neutralTextColor,      text: "white" },
                    { label: "Negative",   color: Kirigami.Theme.negativeTextColor,     text: "white" }
                ]
                delegate: Rectangle {
                    required property var modelData
                    radius: Kirigami.Units.cornerRadius
                    color: modelData.color
                    implicitWidth: label.implicitWidth + Kirigami.Units.largeSpacing * 2
                    implicitHeight: label.implicitHeight + Kirigami.Units.smallSpacing * 2
                    Controls.Label {
                        id: label
                        anchors.centerIn: parent
                        text: modelData.label
                        color: modelData.text
                        font.pixelSize: 11
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
