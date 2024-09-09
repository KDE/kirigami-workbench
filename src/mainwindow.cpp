#include "mainwindow.h"

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

    auto textEdit = new QTextEdit();
    connect(textEdit, &QTextEdit::textChanged, this, [this, textEdit] {
        QTemporaryFile qmlWorkFile;
        qmlWorkFile.open();
        qmlWorkFile.write(textEdit->document()->toPlainText().toUtf8());
        qmlWorkFile.close();

        m_quickWidget->setSource(QUrl{qmlWorkFile.fileName()});
    });
    textEdit->setText(
        QStringLiteral("import QtQuick 2.15\n"
                       "\n"
                       "Rectangle {\n"
                       "    color: \"red\"\n"
                       "    width: 100\n"
                       "    height: 100\n"
                       "}"));
    layout->addWidget(textEdit);
    layout->addWidget(m_quickWidget);

    setupGUI();
}

#include "moc_mainwindow.cpp"