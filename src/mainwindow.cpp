// SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"

#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Message>
#include <KTextEditor/Plugin>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/View>

#include <QQmlEngine>
#include <QSplitter>
#include <QStatusBar>
#include <QThread>

MainWindow::MainWindow()
    : KXmlGuiWindow()
    , m_qmlWorkFile(QStringLiteral("XXXXXX.qml"))
{
    m_qmlWorkFile.open();
    m_qmlWorkFile.write(
        QByteArrayLiteral("import QtQuick 2.15\n"
                          "\n"
                          "Rectangle {\n"
                          "    color: \"red\"\n"
                          "    width: 100\n"
                          "    height: 100\n"
                          "}"));
    m_qmlWorkFile.close();

    auto splitter = new QSplitter();
    setCentralWidget(splitter);

    m_quickWidget = new QQuickWidget();
    m_quickWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);

    m_application = new KTextEditor::Application(this);
    m_mainWindow = new KTextEditor::MainWindow(this);

    auto editor = KTextEditor::Editor::instance();
    KTextEditor::Editor::instance()->setApplication(m_application);

    m_document = editor->createDocument(this);
    m_document->setConfigValue(QStringLiteral("keep-extra-spaces"), true);
    m_document->setConfigValue(QStringLiteral("remove-spaces"), 0);
    connect(m_document, &KTextEditor::Document::textChanged, this, [this](KTextEditor::Document *doc) {
        doc->save();

        m_quickWidget->engine()->clearComponentCache(); // Needed to make sure the engine doesn't cache our temporary file
        m_quickWidget->setSource(QUrl::fromLocalFile(m_qmlWorkFile.fileName()));
    });
    m_document->setHighlightingMode(QStringLiteral("qml"));
    m_document->openUrl(QUrl::fromLocalFile(m_qmlWorkFile.fileName()));

    m_view = m_document->createView(this);

    // Load the LSP plugin
    const QStringList pluginsToLoad = QStringList() << QStringLiteral("lspclientplugin");
    const QList<KPluginMetaData> plugins = KPluginMetaData::findPlugins(QStringLiteral("kf6/ktexteditor"));
    for (const auto &metaData : plugins) {
        const QString identifier = metaData.pluginId();
        if (!pluginsToLoad.contains(identifier)) {
            continue;
        }

        KTextEditor::Plugin *plugin = KPluginFactory::instantiatePlugin<KTextEditor::Plugin>(metaData, this, QVariantList() << identifier).plugin;
        if (plugin) {
            Q_EMIT KTextEditor::Editor::instance() -> application()->pluginCreated(identifier, plugin);
            QObject *created = plugin->createView(m_mainWindow);
            if (created) {
                KTextEditor::SessionConfigInterface *interface = qobject_cast<KTextEditor::SessionConfigInterface *>(created);
                if (interface) {
                    // NOTE: Some plugins will misbehave, unless readSessionConfig has been called!
                    KConfigGroup group = KSharedConfig::openConfig()->group(QStringLiteral("KatePlugin:%1:").arg(identifier));
                    interface->readSessionConfig(group);
                }
            }
        }
    }

    splitter->addWidget(m_view);
    splitter->addWidget(m_quickWidget);
    const auto halfWidth = static_cast<int>(QWidget::width() / 2.0);
    splitter->setSizes({halfWidth, halfWidth});

    // Steal KTextEditor::View's StatusBar
    statusBar()->setSizeGripEnabled(false);
    const auto widgets = m_view->findChildren<QWidget *>(QString(), Qt::FindChildrenRecursively);
    for (auto *widget : widgets) {
        if (widget && widget->metaObject()->className() == QByteArrayLiteral("KateStatusBar")) {
            statusBar()->addPermanentWidget(widget);
            break;
        }
    }

    setupGUI();
}

#include "moc_mainwindow.cpp"
