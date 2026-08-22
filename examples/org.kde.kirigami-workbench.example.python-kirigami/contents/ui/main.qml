// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Kirigami Workbench Contributors

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Item {
    implicitWidth: 480
    implicitHeight: 560

    ColumnLayout {
        anchors { fill: parent; margins: Kirigami.Units.largeSpacing }
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            text: "Python + Kirigami"
            level: 1
            Layout.fillWidth: true
        }

        Controls.Label {
            text: "Edit this file in your favourite editor.\n" +
                  "Changes save automatically and the preview updates in real time."
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            opacity: 0.7
        }

        Kirigami.Separator { Layout.fillWidth: true }

        Kirigami.Heading { text: "Interactive counter"; level: 3 }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            Controls.Button {
                icon.name: "list-remove-symbolic"
                onClicked: counter.value = Math.max(0, counter.value - 1)
            }

            Controls.Label {
                id: counter
                property int value: 0
                text: value
                font.pixelSize: 24
                horizontalAlignment: Text.AlignHCenter
                Layout.minimumWidth: Kirigami.Units.gridUnit * 3
            }

            Controls.Button {
                icon.name: "list-add-symbolic"
                onClicked: counter.value += 1
            }

            Controls.Button {
                text: "Reset"
                flat: true
                onClicked: counter.value = 0
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        Kirigami.Heading { text: "User input"; level: 3 }

        Kirigami.InlineMessage {
            id: feedback
            Layout.fillWidth: true
            visible: text.length > 0
            type: Kirigami.MessageType.Positive
        }

        RowLayout {
            Layout.fillWidth: true

            Controls.TextField {
                id: nameField
                placeholderText: "Enter your name…"
                Layout.fillWidth: true
                Keys.onReturnPressed: greetBtn.clicked()
            }

            Controls.Button {
                id: greetBtn
                text: "Greet"
                highlighted: true
                onClicked: {
                    if (nameField.text.trim().length > 0)
                        feedback.text = "Hello, " + nameField.text.trim() + "! 👋"
                    else
                        feedback.text = ""
                }
            }
        }

        Kirigami.Separator { Layout.fillWidth: true }

        Kirigami.Heading { text: "Status"; level: 3 }

        Kirigami.AbstractCard {
            Layout.fillWidth: true
            contentItem: RowLayout {
                Kirigami.Icon {
                    source: "python"
                    fallback: "text-x-script"
                    implicitWidth:  Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                }
                Controls.Label {
                    text: "Ready to connect Python logic via PySide6 or PyQt6"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
