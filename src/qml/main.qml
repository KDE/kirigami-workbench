// SPDX-FileCopyrightText: 2023 Carl Schwan <carl@carlschwan.eu>
// SPDX-License-Identifier: LGPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.15 as Kirigami
import org.kde.syntaxhighlighting 1.0

Kirigami.ApplicationWindow {
    id: root

    pageStack.initialPage: Kirigami.Page {
        padding: 0
        title: "Workbench"

        QQC2.SplitView {
            anchors.fill: parent

            QQC2.Page {

                QQC2.SplitView.preferredWidth: root.width / 2

                contentItem: QQC2.TextArea {
                    id: code

                    text: "import QtQuick 2.15

Rectangle {
    color: \"red\"
    width: 100
    height: 100
}"

                    SyntaxHighlighter {
                        textEdit: code
                        definition: "QML"
                    }

                    onTextChanged: try {
                        const newContent = Qt.createQmlObject(text, contentPage);
                        if (contentPage.content) {
                            contentPage.content.destroy();
                        }
                        contentPage.content = newContent;
                    } catch (error) {
                        console.error("hello", error)
                    }

                    Kirigami.Theme.colorSet: Kirigami.Theme.View

                    background: Rectangle {
                        color: Kirigami.Theme.backgroundColor
                    }
                }
            }

            QQC2.Page {
                id: contentPage

                property var content: null

                QQC2.SplitView.preferredWidth: root.width / 2

                background: Rectangle {
                    color: Kirigami.Theme.backgroundColor
                }
            }
        }
    }
}

