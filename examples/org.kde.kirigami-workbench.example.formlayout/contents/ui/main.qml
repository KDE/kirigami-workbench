import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Item {
    implicitWidth: 460
    implicitHeight: 500

    Kirigami.FormLayout {
        anchors { fill: parent; margins: Kirigami.Units.largeSpacing }

        Kirigami.Separator { Kirigami.FormData.label: "Account"; Kirigami.FormData.isSection: true }
        Controls.TextField { Kirigami.FormData.label: "Display name:"; placeholderText: "Your name" }
        Controls.TextField { Kirigami.FormData.label: "Email:";        placeholderText: "user@example.com" }

        Kirigami.Separator { Kirigami.FormData.label: "Appearance"; Kirigami.FormData.isSection: true }
        Controls.ComboBox { Kirigami.FormData.label: "Theme:";     model: ["System Default", "Breeze Light", "Breeze Dark"] }
        Controls.SpinBox  { Kirigami.FormData.label: "Font size:"; from: 8; to: 32; value: 10 }

        Kirigami.Separator { Kirigami.FormData.label: "Behaviour"; Kirigami.FormData.isSection: true }
        Controls.CheckBox { Kirigami.FormData.label: "Notifications:"; text: "Enable desktop notifications"; checked: true }
        Controls.CheckBox { Kirigami.FormData.label: "";               text: "Play notification sounds" }
        Controls.Switch   { Kirigami.FormData.label: "Auto-save:";     checked: false }
        Controls.Slider   { Kirigami.FormData.label: "Volume:";        from: 0; to: 100; value: 75 }
    }
}
