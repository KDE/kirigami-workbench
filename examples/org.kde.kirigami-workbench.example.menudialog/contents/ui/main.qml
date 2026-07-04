import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs

Item {
    id: root
    implicitWidth: 320
    implicitHeight: 200

    ColumnLayout {
        anchors { fill: parent; margins: Kirigami.Units.largeSpacing }

        Kirigami.InlineMessage {
            id: msg
            Layout.fillWidth: true
            visible: text.length > 0
        }

        Controls.Button {
            text: "Track options\u2026"
            icon.name: "overflow-menu-symbolic"
            onClicked: menuDlg.open()
        }

        Item { Layout.fillHeight: true }
    }

    KirigamiDialogs.MenuDialog {
        id: menuDlg
        parent: root
        title: "Track Options"
        actions: [
            Kirigami.Action { icon.name: "media-playback-start-symbolic"; text: "Play";           onTriggered: msg.text = "Play" },
            Kirigami.Action { icon.name: "media-skip-forward-symbolic";   text: "Play next";      onTriggered: msg.text = "Play next" },
            Kirigami.Action { icon.name: "list-add-symbolic";             text: "Add to queue";   onTriggered: msg.text = "Add to queue" },
            Kirigami.Action { icon.name: "documentinfo-symbolic";         text: "View details";   onTriggered: msg.text = "View details" },
            Kirigami.Action { icon.name: "document-open-folder-symbolic"; text: "Show in folder"; enabled: false }
        ]
    }
}
