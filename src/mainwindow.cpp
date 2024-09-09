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
#include <QToolTip>

MainWindow::MainWindow()
    : KXmlGuiWindow()
    , qmlWorkFile(QStringLiteral("XXXXXX.qml"))
{
    qmlWorkFile.open();
    qmlWorkFile.write(
        QByteArrayLiteral("import QtQuick 2.15\n"
                          "\n"
                          "Rectangle {\n"
                          "    color: \"red\"\n"
                          "    width: 100\n"
                          "    height: 100\n"
                          "}"));
    qmlWorkFile.close();

    auto splitter = new QSplitter();
    setCentralWidget(splitter);

    m_quickWidget = new QQuickWidget();
    m_quickWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);

    m_application = new KTextEditor::Application(this);
    m_mainWindow = new KTextEditor::MainWindow(this);

    auto editor = KTextEditor::Editor::instance();
    KTextEditor::Editor::instance()->setApplication(m_application);

    m_doc = editor->createDocument(this);
    m_doc->setConfigValue(QStringLiteral("keep-extra-spaces"), true);
    m_doc->setConfigValue(QStringLiteral("remove-spaces"), 0);
    connect(m_doc, &KTextEditor::Document::textChanged, this, [this](KTextEditor::Document *doc) {
        doc->save();

        m_quickWidget->engine()->clearComponentCache(); // Needed to make sure the engine doesn't cache our temporary file
        m_quickWidget->setSource(QUrl::fromLocalFile(qmlWorkFile.fileName()));
    });
    m_doc->setHighlightingMode(QStringLiteral("qml"));
    m_doc->openUrl(QUrl::fromLocalFile(qmlWorkFile.fileName()));

    m_view = m_doc->createView(this);

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
