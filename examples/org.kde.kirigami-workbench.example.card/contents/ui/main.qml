import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Item {
    implicitWidth: 420
    implicitHeight: 600

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: col.implicitHeight + Kirigami.Units.largeSpacing * 2
        clip: true

        ColumnLayout {
            id: col
            width: parent.width - Kirigami.Units.largeSpacing * 2
            x: Kirigami.Units.largeSpacing
            y: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

            Kirigami.Heading { text: "AbstractCard"; level: 3 }
            Kirigami.AbstractCard {
                Layout.fillWidth: true
                header: Kirigami.Heading { text: "AbstractCard"; level: 2 }
                contentItem: Controls.Label {
                    wrapMode: Text.WordWrap
                    text: "Simplest card type — a rectangle with shadow. Any Item can be its header or contentItem."
                }
            }

            Kirigami.Heading { text: "Card (banner + actions)"; level: 3 }
            Kirigami.Card {
                Layout.fillWidth: true
                banner { title: "Title Alignment"; titleAlignment: Qt.AlignLeft | Qt.AlignBottom }
                contentItem: Controls.Label {
                    wrapMode: Text.WordWrap
                    text: "Card adds a banner (image + title overlay) and an actions footer with ToolButtons."
                }
                actions: [
                    Kirigami.Action { text: "Add item";  icon.name: "list-add-symbolic" },
                    Kirigami.Action { text: "Edit item"; icon.name: "document-edit-symbolic" }
                ]
            }

            Kirigami.Heading { text: "CardsLayout"; level: 3 }
            Kirigami.CardsLayout {
                Layout.fillWidth: true
                Kirigami.Card { contentItem: Controls.Label { wrapMode: Text.WordWrap; text: "Card in grid" } }
                Kirigami.AbstractCard { contentItem: Controls.Label { wrapMode: Text.WordWrap; text: "AbstractCard in grid" } }
                Kirigami.Card { headerOrientation: Qt.Horizontal; contentItem: Controls.Label { wrapMode: Text.WordWrap; text: "Horizontal Card" } }
            }
        }
    }
}
