#include "mainwindow.h"

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

#include <QHBoxLayout>
#include <QTemporaryFile>
#include <QTextEdit>

MainWindow::MainWindow()
    : KXmlGuiWindow()
{
    auto widget = new QWidget();
    setCentralWidget(widget);

    auto layout = new QHBoxLayout();
    widget->setLayout(layout);

    m_quickWidget = new QQuickWidget();

    auto editor = KTextEditor::Editor::instance();
    auto doc = editor->createDocument(this);
    connect(doc, &KTextEditor::Document::textChanged, this, [this](KTextEditor::Document *doc) {
        QTemporaryFile qmlWorkFile;
        qmlWorkFile.open();
        qmlWorkFile.write(doc->text().toUtf8());
        qmlWorkFile.close();

        m_quickWidget->setSource(QUrl{qmlWorkFile.fileName()});
    });
    doc->setText(
        QStringLiteral("import QtQuick 2.15\n"
                       "\n"
                       "Rectangle {\n"
                       "    color: \"red\"\n"
                       "    width: 100\n"
                       "    height: 100\n"
                       "}"));
    auto view = doc->createView(this);

    layout->addWidget(view);
    layout->addWidget(m_quickWidget);

    setupGUI();
}

#include "moc_mainwindow.cpp"