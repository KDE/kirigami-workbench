import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigami.dialogs as KirigamiDialogs

Item {
    id: root
    implicitWidth: 380
    implicitHeight: 240

    ColumnLayout {
        anchors { fill: parent; margins: Kirigami.Units.largeSpacing }

        Kirigami.InlineMessage {
            id: msg
            Layout.fillWidth: true
            visible: text.length > 0
        }

        Controls.Button {
            text: "Choose a Planet\u2026"
            onClicked: dlg.open()
        }

        Item { Layout.fillHeight: true }
    }

    KirigamiDialogs.Dialog {
        id: dlg
        parent: root
        title: "Choose a Planet"
        preferredWidth: Kirigami.Units.gridUnit * 18
        standardButtons: Controls.DialogButtonBox.Ok | Controls.DialogButtonBox.Cancel
        customFooterActions: [
            Kirigami.Action {
                text: "Random"
                icon.name: "roll-symbolic"
                onTriggered: planetList.currentIndex = Math.floor(Math.random() * planetList.model.length)
            }
        ]
        onAccepted: msg.text = "Accepted: " + planetList.model[planetList.currentIndex]
        onRejected: msg.text = "Cancelled"

        ListView {
            id: planetList
            implicitWidth: Kirigami.Units.gridUnit * 16
            implicitHeight: Kirigami.Units.gridUnit * 14
            currentIndex: 0
            model: ["Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"]
            delegate: Controls.RadioDelegate {
                required property string modelData
                required property int index
                implicitWidth: planetList.width
                text: modelData
                checked: planetList.currentIndex === index
                onClicked: planetList.currentIndex = index
            }
        }
    }
}
