// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
#include "componentproject.h"
#include "examplemodel.h"
#include "tabmanager.h"
#include "tabpage.h"
#include "welcomepage.h"
#include "workbenchviewspace.h"

#include <KActionCollection>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/View>

#include <KLocalizedString>
#include <QInputDialog>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabBar>

using namespace Qt::StringLiterals;

MainWindow::MainWindow()
    : KXmlGuiWindow()
{
    m_application = new KTextEditor::Application(this);
    m_mainWindow = new KTextEditor::MainWindow(this);

    KTextEditor::Editor::instance()->setApplication(m_application);

    m_project = new ComponentProject(this);

    m_exampleModel = new ExampleModel(this);

    m_viewSpace = new WorkbenchViewSpace(m_project, m_exampleModel, this);

    m_tabManager = new TabManager(m_viewSpace, this);

    m_welcomePage = new WelcomePage(m_exampleModel, this);
    m_centralStack = new QStackedWidget(this);
    m_centralStack->addWidget(m_welcomePage);
    m_centralStack->addWidget(m_viewSpace);
    setCentralWidget(m_centralStack);

    connect(m_welcomePage, &WelcomePage::startFromScratch, this, [this]() {
        m_viewSpace->resetToBlank();
        showWorkbench();
    });
    connect(m_welcomePage, &WelcomePage::exampleSelected, this, [this](const QString &pluginId) {
        showWorkbench();
        m_viewSpace->openExample(pluginId);
    });

    m_centralStack->setCurrentIndex(0);

    const QStringList pluginsToLoad = QStringList() << u"lspclientplugin"_s;
    const QList<KPluginMetaData> plugins = KPluginMetaData::findPlugins(u"kf6/ktexteditor"_s);
    for (const auto &metaData : plugins) {
        const QString identifier = metaData.pluginId();
        if (!pluginsToLoad.contains(identifier))
            continue;

        KTextEditor::Plugin *plugin = KPluginFactory::instantiatePlugin<KTextEditor::Plugin>(metaData, this, QVariantList() << identifier).plugin;
        if (plugin) {
            Q_EMIT KTextEditor::Editor::instance() -> application()->pluginCreated(identifier, plugin);
            QObject *created = plugin->createView(m_mainWindow);
            if (created) {
                KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(created);
                if (iface) {
                    KConfigGroup group = KSharedConfig::openConfig()->group(u"KatePlugin:%1:"_s.arg(identifier));
                    iface->readSessionConfig(group);
                }
            }
        }
    }

    setupGUI(Keys | StatusBar | Save | Create);
    setupActions();

    KConfigGroup galleryConfig = KSharedConfig::openConfig()->group(u"GalleryPanel"_s);
    const bool galleryVisible = galleryConfig.readEntry("visible", true);
    m_viewSpace->setGalleryVisible(galleryVisible);
    if (m_toggleGalleryAction)
        m_toggleGalleryAction->setChecked(galleryVisible);

    statusBar()->setSizeGripEnabled(false);
    statusBar()->hide();

    action(u"help_contents"_s)->setVisible(false);

    QAction *galleryAction = actionCollection()->action(u"view_welcome"_s);
    galleryAction->setVisible(true);

    QMenuBar *mb = menuBar();
    if (mb) {
        QAction *firstMenu = mb->actions().isEmpty() ? nullptr : mb->actions().first();
        mb->insertAction(firstMenu, galleryAction);
    }

    connect(m_centralStack, &QStackedWidget::currentChanged, this, [galleryAction](int index) {
        galleryAction->setVisible(index == 1);
    });
    galleryAction->setVisible(false);
}

KTextEditor::View *MainWindow::activeView()
{
    return nullptr;
}

QVariantMap MainWindow::projectMapForDocument(KTextEditor::Document *doc)
{
    if (m_tabManager)
        return m_tabManager->projectMapForDocument(doc);
    return QVariantMap();
}

KTextEditor::Document *MainWindow::findUrl(const QUrl &url)
{
    Q_UNUSED(url)
    return nullptr;
}

void MainWindow::setupActions()
{
    QAction *saveAction = actionCollection()->addAction(u"file_save"_s);
    saveAction->setText(i18n("Save"));
    saveAction->setIcon(QIcon::fromTheme(u"document-save-symbolic"_s));
    actionCollection()->setDefaultShortcut(saveAction, Qt::CTRL | Qt::Key_S);
    connect(saveAction, &QAction::triggered, this, [this]() {
        TabPage *tab = m_viewSpace->currentTab();
        if (tab)
            tab->save();
    });

    QAction *newFileAction = actionCollection()->addAction(u"file_new_component"_s);
    newFileAction->setText(i18n("New component\u2026"));
    newFileAction->setIcon(QIcon::fromTheme(u"document-new-symbolic"_s));
    actionCollection()->setDefaultShortcut(newFileAction, Qt::CTRL | Qt::Key_N);
    connect(newFileAction, &QAction::triggered, this, [this]() {
        if (m_project && m_viewSpace) {
            bool ok;
            QString name =
                QInputDialog::getText(this, i18n("New Component"), i18n("Component name (without .qml extension):"), QLineEdit::Normal, QString(), &ok);
            if (ok && !name.isEmpty()) {
                QString path = m_project->addSubComponent(name);
                if (!path.isEmpty()) {
                    m_viewSpace->updateFileList();
                    m_viewSpace->openFile(path);
                }
            }
        }
    });

    QAction *closeTabAction = actionCollection()->addAction(u"file_close_tab"_s);
    closeTabAction->setText(i18n("Close tab"));
    closeTabAction->setIcon(QIcon::fromTheme(u"tab-close-symbolic"_s));
    actionCollection()->setDefaultShortcuts(closeTabAction, {Qt::CTRL | Qt::Key_W, Qt::CTRL | Qt::Key_F4});
    connect(closeTabAction, &QAction::triggered, this, [this]() {
        int currentIndex = m_viewSpace->tabCount() > 0 ? qobject_cast<QTabBar *>(m_viewSpace->findChild<QTabBar *>())->currentIndex() : -1;
        if (currentIndex >= 0)
            m_viewSpace->closeTab(currentIndex);
    });

    QAction *nextTabAction = actionCollection()->addAction(u"next_tab"_s);
    nextTabAction->setText(i18n("Next tab"));
    actionCollection()->setDefaultShortcuts(nextTabAction, {Qt::CTRL | Qt::Key_Tab, Qt::CTRL | Qt::Key_PageDown});
    connect(nextTabAction, &QAction::triggered, this, [this]() {
        m_viewSpace->focusNextTab();
    });

    QAction *prevTabAction = actionCollection()->addAction(u"previous_tab"_s);
    prevTabAction->setText(i18n("Previous tab"));
    actionCollection()->setDefaultShortcuts(prevTabAction, {Qt::CTRL | Qt::SHIFT | Qt::Key_Tab, Qt::CTRL | Qt::Key_PageUp});
    connect(prevTabAction, &QAction::triggered, this, [this]() {
        m_viewSpace->focusPrevTab();
    });

    for (int i = 1; i <= 9; ++i) {
        QAction *tabAction = actionCollection()->addAction(u"switch_to_tab_%1"_s.arg(i));
        tabAction->setText(i18n("Switch to Tab %1", i));
        actionCollection()->setDefaultShortcut(tabAction, QKeySequence(Qt::ALT | (Qt::Key_0 + i)));
        connect(tabAction, &QAction::triggered, this, [this, i]() {
            m_viewSpace->focusTab(i - 1);
        });
    }

    m_toggleGalleryAction = actionCollection()->addAction(u"toggle_gallery"_s);
    m_toggleGalleryAction->setText(i18n("Show gallery"));
    m_toggleGalleryAction->setCheckable(true);
    m_toggleGalleryAction->setChecked(true);
    m_toggleGalleryAction->setIcon(QIcon::fromTheme(u"view-list-symbolic"_s));
    actionCollection()->setDefaultShortcut(m_toggleGalleryAction, Qt::CTRL | Qt::Key_G);
    connect(m_toggleGalleryAction, &QAction::toggled, this, [this](bool checked) {
        m_viewSpace->setGalleryVisible(checked);
    });

    QAction *backToWelcomeAction = actionCollection()->addAction(u"view_welcome"_s);
    backToWelcomeAction->setText(i18n("Back to gallery"));
    backToWelcomeAction->setIcon(QIcon::fromTheme(u"go-home-symbolic"_s));
    actionCollection()->setDefaultShortcut(backToWelcomeAction, Qt::CTRL | Qt::Key_Home);
    connect(backToWelcomeAction, &QAction::triggered, this, [this]() {
        m_centralStack->setCurrentIndex(0);
    });
}

void MainWindow::showWorkbench()
{
    if (m_centralStack->currentIndex() == 1)
        return;

    if (m_viewSpace->tabCount() == 0 && m_project && !m_project->mainComponentPath().isEmpty()) {
        m_viewSpace->openFile(m_project->mainComponentPath());
        m_viewSpace->updateFileList();
        m_viewSpace->reloadPreview();
    }

    m_centralStack->setCurrentIndex(1);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    KConfigGroup galleryConfig = KSharedConfig::openConfig()->group(u"GalleryPanel"_s);
    galleryConfig.writeEntry("visible", m_viewSpace->isGalleryVisible());
    galleryConfig.sync();
    KXmlGuiWindow::closeEvent(event);
}

#include "moc_mainwindow.cpp"
