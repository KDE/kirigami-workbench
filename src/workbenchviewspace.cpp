// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "workbenchviewspace.h"
#include "componentproject.h"
#include "examplemodel.h"
#include "gallerypanel.h"
#include "tabpage.h"
#include "workbenchtabbar.h"

#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>

#include <KTextEditor/Cursor>
#include <KTextEditor/Range>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonObject>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QRegularExpression>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>
using namespace Qt::StringLiterals;

static QTextEdit *s_consoleWidget = nullptr;
static QtMessageHandler s_previousHandler = nullptr;

static void qmlMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    if (s_previousHandler)
        s_previousHandler(type, ctx, msg);

    if (!s_consoleWidget)
        return;

    if (type != QtWarningMsg && type != QtCriticalMsg)
        return;

    const QString category = ctx.category ? QString::fromUtf8(ctx.category) : QString();
    const bool isQml =
        category.startsWith(QLatin1String("qml")) || category.startsWith(QLatin1String("js")) || msg.contains(QRegularExpression(u"\\.qml[?:]"_s));
    if (!isQml)
        return;

    if (msg.contains(u"_preview_wrapper.qml"_s))
        return;

    QString display = msg;
    static const QRegularExpression fileUrlRe(u"file:///[^?]+/([^/?]+\\.qml)(\\?[^:]*)?"_s);
    display.replace(fileUrlRe, u"\\1"_s);

    const QString errorColor = s_consoleWidget ? s_consoleWidget->palette().color(QPalette::Active, QPalette::HighlightedText).name() : u"#f28b82"_s;
    QMetaObject::invokeMethod(s_consoleWidget,
                              "append",
                              Qt::QueuedConnection,
                              Q_ARG(QString, u"<span style='color:"_s + errorColor + u"'>error: </span>"_s + display.toHtmlEscaped()));
}

WorkbenchViewSpace::WorkbenchViewSpace(ComponentProject *project, ExampleModel *exampleModel, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_exampleModel(exampleModel)
{
    setupUI();
    setupConnections();
}

WorkbenchViewSpace::~WorkbenchViewSpace()
{
    qInstallMessageHandler(s_previousHandler);
    s_consoleWidget = nullptr;
    s_previousHandler = nullptr;
}

void WorkbenchViewSpace::log(const QString &msg)
{
    if (m_consoleWidget)
        m_consoleWidget->append(msg);
}

void WorkbenchViewSpace::setupUI()
{
    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    m_tabBar = new WorkbenchTabBar(this);
    m_stack = new QStackedWidget(this);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);

    m_galleryPanel = new GalleryPanel(m_exampleModel, this);
    m_mainSplitter->addWidget(m_galleryPanel);

    m_fileList = new QListWidget(this);
    m_fileList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_fileList, &QListWidget::customContextMenuRequested, this, &WorkbenchViewSpace::showFileListContextMenu);
    m_mainSplitter->addWidget(m_fileList);

    auto *rightSplitter = new QSplitter(Qt::Vertical, this);
    auto *topSplitter = new QSplitter(Qt::Horizontal, this);

    auto *editorContainer = new QWidget(this);
    auto *editorLayout = new QVBoxLayout(editorContainer);

    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    auto *tabBarRow = new QWidget(this);
    auto *tabBarLayout = new QHBoxLayout(tabBarRow);

    tabBarLayout->setContentsMargins(0, 0, 0, 0);
    tabBarLayout->setSpacing(0);
    tabBarLayout->addWidget(m_tabBar, 1);

    editorLayout->addWidget(tabBarRow);
    editorLayout->addWidget(m_stack, 1);

    topSplitter->addWidget(editorContainer);

    auto *previewEngine = new QQmlEngine(this);

    QPalette lightPalette = QApplication::palette();
    lightPalette.setColor(QPalette::All, QPalette::Window, QColor(0xFCFCFC));
    lightPalette.setColor(QPalette::All, QPalette::Base, Qt::white);
    lightPalette.setColor(QPalette::All, QPalette::Button, QColor(0xEEF0F2));
    lightPalette.setColor(QPalette::All, QPalette::WindowText, QColor(0x232629));
    lightPalette.setColor(QPalette::All, QPalette::Text, QColor(0x232629));
    lightPalette.setColor(QPalette::All, QPalette::ButtonText, QColor(0x232629));
    lightPalette.setColor(QPalette::All, QPalette::Highlight, QColor(0x3DAEE9));
    lightPalette.setColor(QPalette::All, QPalette::HighlightedText, Qt::white);

    m_previewWidget = new QQuickWidget(previewEngine, this);
    m_previewWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_previewWidget->setMinimumSize(200, 200);
    m_previewWidget->setClearColor(Qt::white);
    m_previewWidget->setPalette(lightPalette);
    topSplitter->addWidget(m_previewWidget);

    topSplitter->setStretchFactor(0, 4);
    topSplitter->setStretchFactor(1, 1);

    rightSplitter->addWidget(topSplitter);

    m_consoleWidget = new QTextEdit(this);
    m_consoleWidget->setReadOnly(true);
    m_consoleWidget->setPlaceholderText(u"Console output will appear here"_s);
    m_consoleWidget->setMinimumHeight(80);
    rightSplitter->addWidget(m_consoleWidget);

    s_consoleWidget = m_consoleWidget;
    s_previousHandler = qInstallMessageHandler(qmlMessageHandler);

    rightSplitter->setStretchFactor(0, 7);
    rightSplitter->setStretchFactor(1, 3);

    m_mainSplitter->addWidget(rightSplitter);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 7);

    connect(m_galleryPanel, &GalleryPanel::exampleSelected, this, &WorkbenchViewSpace::openExample);

    m_layout->addWidget(m_mainSplitter, 0, 0, 1, 1);
}

void WorkbenchViewSpace::setupConnections()
{
    connect(m_tabBar, &QTabBar::currentChanged, this, &WorkbenchViewSpace::changeTab);

    connect(m_tabBar, &WorkbenchTabBar::newTabRequested, this, [this]() {
        if (m_project)
            openFile(m_project->mainComponentPath());
    });

    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &WorkbenchViewSpace::closeTab);
    connect(m_tabBar, &WorkbenchTabBar::contextMenuRequest, this, &WorkbenchViewSpace::showTabContextMenu);

    connect(m_fileList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty())
            openFile(path);
    });

    if (m_project) {
        connect(m_project, &ComponentProject::fileAdded, this, &WorkbenchViewSpace::updateFileList);
        connect(m_project, &ComponentProject::fileRemoved, this, &WorkbenchViewSpace::updateFileList);
    }

    connect(m_previewWidget, &QQuickWidget::statusChanged, this, [this](QQuickWidget::Status status) {
        if (status == QQuickWidget::Ready)
            m_consoleWidget->clear();
    });
}

TabPage *WorkbenchViewSpace::openFile(const QString &filePath)
{
    const QString absolutePath = QFileInfo(filePath).canonicalFilePath();
    if (absolutePath.isEmpty())
        return nullptr;

    for (int i = 0; i < m_stack->count(); ++i) {
        TabPage *tab = qobject_cast<TabPage *>(m_stack->widget(i));
        if (!tab)
            continue;
        const QString tabPath = QFileInfo(tab->filePath()).canonicalFilePath();
        if (tabPath.compare(absolutePath,
#ifdef Q_OS_WIN
                Qt::CaseInsensitive
#else
                Qt::CaseSensitive
#endif
            ) == 0) {
            m_tabBar->setCurrentIndex(i);
            m_stack->setCurrentIndex(i);
            return tab;
        }
    }

    KTextEditor::Document *doc = getOrCreateDocument(absolutePath);
    if (!doc) {
        qWarning() << "Failed to create document for:" << absolutePath;
        return nullptr;
    }

    return createTabForFile(absolutePath, doc);
}

TabPage *WorkbenchViewSpace::createTabForFile(const QString &filePath, KTextEditor::Document *doc)
{
    if (!doc) {
        qWarning() << "Cannot create tab with null document";
        return nullptr;
    }

    auto *tabPage = new TabPage(filePath, doc, this);

    m_stack->addWidget(tabPage);

    QString fileName = QFileInfo(filePath).fileName();
    int tabIndex = m_tabBar->addTab(fileName);

    m_fileToTab[filePath] = tabPage;

    m_tabBar->setCurrentIndex(tabIndex);
    m_stack->setCurrentWidget(tabPage);

    connect(tabPage, &TabPage::modifiedChanged, this, [this, tabIndex](bool modified) {
        m_tabBar->setModifiedStateIcon(tabIndex, modified);
    });

    connect(tabPage, &QObject::destroyed, this, [this, filePath]() {
        m_fileToTab.remove(filePath);
    });

    Q_EMIT currentTabChanged(tabPage);
    return tabPage;
}

TabPage *WorkbenchViewSpace::currentTab() const
{
    return qobject_cast<TabPage *>(m_stack->currentWidget());
}

void WorkbenchViewSpace::closeTab(int index)
{
    if (index < 0 || index >= m_tabBar->count())
        return;

    TabPage *tabPage = qobject_cast<TabPage *>(m_stack->widget(index));
    if (!tabPage)
        return;

    if (tabPage->isModified()) {
        QString fileName = QFileInfo(tabPage->filePath()).fileName();
        auto result = QMessageBox::question(this,
                                            tr("Unsaved Changes"),
                                            tr("Save changes to %1?").arg(fileName),
                                            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if (result == QMessageBox::Cancel)
            return;
        if (result == QMessageBox::Save)
            tabPage->save();
    }

    QString filePath = tabPage->filePath();
    m_fileToTab.remove(filePath);
    m_tabBar->removeTab(index);
    m_stack->removeWidget(tabPage);
    tabPage->deleteLater();

    if (m_tabBar->count() == 0 && m_project)
        openFile(m_project->mainComponentPath());
}

int WorkbenchViewSpace::tabCount() const
{
    return m_tabBar->count();
}

void WorkbenchViewSpace::focusPrevTab()
{
    int idx = m_tabBar->currentIndex();
    if (idx > 0)
        m_tabBar->setCurrentIndex(idx - 1);
    else if (m_tabBar->count() > 1)
        m_tabBar->setCurrentIndex(m_tabBar->count() - 1);
}

void WorkbenchViewSpace::focusNextTab()
{
    int idx = m_tabBar->currentIndex();
    if (idx < m_tabBar->count() - 1)
        m_tabBar->setCurrentIndex(idx + 1);
    else if (m_tabBar->count() > 1)
        m_tabBar->setCurrentIndex(0);
}

void WorkbenchViewSpace::focusTab(int id)
{
    if (id >= 0 && id < m_tabBar->count())
        m_tabBar->setCurrentIndex(id);
}

void WorkbenchViewSpace::changeTab(int idx)
{
    if (idx >= 0 && idx < m_stack->count()) {
        m_stack->setCurrentIndex(idx);
        TabPage *tab = currentTab();
        if (tab)
            Q_EMIT currentTabChanged(tab);
    }
}

QVariantMap WorkbenchViewSpace::projectMapForDocument(KTextEditor::Document *doc)
{
    Q_UNUSED(doc);
    if (!m_project)
        return QVariantMap();

    QString workingDir = m_project->workingDirectory();
    if (workingDir.isEmpty())
        return QVariantMap();

    QJsonObject lspConfig;
    lspConfig[u"root"_s] = workingDir;

    QJsonObject serversConfig;
    serversConfig[u"qml"_s] = lspConfig;

    QJsonObject lspClientConfig;
    lspClientConfig[u"servers"_s] = serversConfig;

    QJsonObject result;
    result[u"lspclient"_s] = lspClientConfig;
    return result.toVariantMap();
}

void WorkbenchViewSpace::showTabContextMenu(int tabIndex, const QPoint &globalPos)
{
    QMenu menu(this);

    QAction *newTabAction = menu.addAction(QIcon::fromTheme(u"tab-new-symbolic"_s), tr("New tab"));
    connect(newTabAction, &QAction::triggered, this, [this]() {
        if (m_project)
            openFile(m_project->mainComponentPath());
    });

    QAction *closeTabAction = menu.addAction(QIcon::fromTheme(u"tab-close-symbolic"_s), tr("Close tab"));
    connect(closeTabAction, &QAction::triggered, this, [this, tabIndex]() {
        closeTab(tabIndex);
    });

    menu.addSeparator();

    QAction *closeOthersAction = menu.addAction(tr("Close other tabs"));
    closeOthersAction->setEnabled(m_tabBar->count() > 1);
    connect(closeOthersAction, &QAction::triggered, this, [this, tabIndex]() {
        for (int i = m_tabBar->count() - 1; i >= 0; --i)
            if (i != tabIndex)
                closeTab(i);
    });

    QAction *closeRightAction = menu.addAction(tr("Close tabs to the right"));
    closeRightAction->setEnabled(tabIndex < m_tabBar->count() - 1);
    connect(closeRightAction, &QAction::triggered, this, [this, tabIndex]() {
        for (int i = m_tabBar->count() - 1; i > tabIndex; --i)
            closeTab(i);
    });

    menu.exec(globalPos);
}

void WorkbenchViewSpace::showFileListContextMenu(const QPoint &pos)
{
    QMenu menu(this);

    QAction *newComponentAction = menu.addAction(QIcon::fromTheme(u"document-new-symbolic"_s), tr("New component\u2026"));
    connect(newComponentAction, &QAction::triggered, this, [this]() {
        if (!m_project)
            return;
        bool ok;
        QString componentName =
            QInputDialog::getText(this, tr("New Component"), tr("Component name (without .qml extension):"), QLineEdit::Normal, QString(), &ok);
        if (ok && !componentName.isEmpty()) {
            QString newFilePath = m_project->addSubComponent(componentName);
            if (!newFilePath.isEmpty()) {
                updateFileList();
                openFile(newFilePath);
            } else {
                QMessageBox::warning(this,
                                     tr("Failed to Create Component"),
                                     tr("Could not create component '%1'. Maximum file limit may have been reached.").arg(componentName));
            }
        }
    });

    QListWidgetItem *selectedItem = m_fileList->itemAt(pos);
    if (selectedItem) {
        QString filePath = selectedItem->data(Qt::UserRole).toString();
        menu.addSeparator();

        QAction *deleteAction = menu.addAction(QIcon::fromTheme(u"edit-delete-symbolic"_s), tr("Delete component"));
        deleteAction->setEnabled(m_project && m_project->canRemove(filePath));

        connect(deleteAction, &QAction::triggered, this, [this, filePath]() {
            QString fileName = QFileInfo(filePath).fileName();
            auto result = QMessageBox::question(this,
                                                tr("Delete Component"),
                                                tr("Are you sure you want to delete %1?").arg(fileName),
                                                QMessageBox::Yes | QMessageBox::No);
            if (result == QMessageBox::Yes && m_project && m_project->removeSubComponent(filePath)) {
                if (m_fileToTab.contains(filePath)) {
                    TabPage *tabPage = m_fileToTab[filePath];
                    int tabIndex = m_stack->indexOf(tabPage);
                    if (tabIndex >= 0)
                        closeTab(tabIndex);
                }
                m_documents.remove(filePath);
                updateFileList();
            }
        });
    }

    menu.exec(m_fileList->mapToGlobal(pos));
}

KTextEditor::Document *WorkbenchViewSpace::getOrCreateDocument(const QString &filePath)
{
    const QString absolutePath = QFileInfo(filePath).canonicalFilePath();
    if (absolutePath.isEmpty())
        return nullptr;

    if (m_documents.contains(absolutePath))
        return m_documents[absolutePath];

    auto *doc = KTextEditor::Editor::instance()->createDocument(this);

    doc->setHighlightingMode(u"QML"_s);
    doc->setMode(u"QML"_s);
    doc->setConfigValue(u"auto-brackets"_s, true);
    doc->setConfigValue(u"indent-mode"_s, u"cstyle"_s);
    doc->setConfigValue(u"indent-width"_s, 4);
    doc->setConfigValue(u"replace-tabs"_s, true);
    doc->setConfigValue(u"tab-width"_s, 4);

    QFile loadFile(absolutePath);
    if (loadFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QSignalBlocker blocker(doc);
        doc->setText(QString::fromUtf8(loadFile.readAll()));
        doc->setModified(false);
        loadFile.close();
    }

    m_documents[absolutePath] = doc;

    QTimer *autoSaveTimer = new QTimer(doc);
    autoSaveTimer->setSingleShot(true);
    autoSaveTimer->setInterval(300);

    QString savedPath = absolutePath;

    connect(doc, &KTextEditor::Document::textInserted, this, [autoSaveTimer](KTextEditor::Document *, const KTextEditor::Cursor &, const QString &) {
        autoSaveTimer->start();
    });
    connect(doc, &KTextEditor::Document::textRemoved, this, [autoSaveTimer](KTextEditor::Document *, const KTextEditor::Range &, const QString &) {
        autoSaveTimer->start();
    });
    connect(doc, &KTextEditor::Document::textChanged, this, [autoSaveTimer](KTextEditor::Document *) {
        autoSaveTimer->start();
    });

    connect(autoSaveTimer, &QTimer::timeout, this, [this, doc, savedPath]() {
        if (!doc || savedPath.isEmpty())
            return;
        QFile file(savedPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QByteArray content = doc->text().toUtf8();
            file.write(content);
            file.flush();
            file.close();
            {
                QSignalBlocker blocker(doc);
                doc->setModified(false);
            }
            if (m_fileToTab.contains(savedPath)) {
                TabPage *tab = m_fileToTab[savedPath];
                int idx = m_stack->indexOf(tab);
                if (idx >= 0)
                    m_tabBar->setModifiedStateIcon(idx, false);
            }
            reloadPreview();
        } else {
            log(u"Auto-save failed: "_s + file.errorString());
        }
    });

    return doc;
}

void WorkbenchViewSpace::updateFileList()
{
    if (!m_fileList || !m_project)
        return;

    m_fileList->clear();

    QString mainPath = m_project->mainComponentPath();
    if (!mainPath.isEmpty()) {
        const QString canonicalMain = QFileInfo(mainPath).canonicalFilePath();
        const QString displayPath = canonicalMain.isEmpty() ? mainPath : canonicalMain;
        auto *item = new QListWidgetItem(QFileInfo(displayPath).fileName());
        item->setData(Qt::UserRole, displayPath);
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        m_fileList->addItem(item);
    }

    for (const QString &path : m_project->subComponentPaths()) {
        if (QFileInfo(path).fileName().startsWith(QLatin1Char('_')))
            continue;
        const QString canonical = QFileInfo(path).canonicalFilePath();
        const QString displayPath = canonical.isEmpty() ? path : canonical;
        auto *item = new QListWidgetItem(QFileInfo(displayPath).fileName());
        item->setData(Qt::UserRole, displayPath);
        m_fileList->addItem(item);
    }
}

void WorkbenchViewSpace::reloadPreview()
{
    if (!m_previewWidget || !m_project)
        return;

    QString mainPath = m_project->mainComponentPath();
    if (mainPath.isEmpty())
        return;

    QFileInfo fileInfo(mainPath);
    if (!fileInfo.exists() || !fileInfo.isReadable())
        return;

    QQmlEngine *engine = m_previewWidget->engine();
    if (!engine)
        return;

    QString workingDir = m_project->workingDirectory();
    if (!workingDir.isEmpty()) {
        QString absWorkingDir = QDir(workingDir).absolutePath();
        auto importPaths = engine->importPathList();
        if (!importPaths.contains(absWorkingDir))
            engine->addImportPath(absWorkingDir);
    }

    engine->clearComponentCache();

    QString wrapperPath = QDir(workingDir).filePath(u"_preview_wrapper.qml"_s);

    QFile wrapper(wrapperPath);
    if (wrapper.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&wrapper);
        QString mainUrl =
            QUrl::fromLocalFile(fileInfo.absoluteFilePath()).toString() + u"?t="_s + QString::number(QDateTime::currentMSecsSinceEpoch());
        out << "import QtQuick\n"
            << "import QtQuick.Controls as QQC2\n"
            << "import org.kde.kirigami as Kirigami\n"
            << "Rectangle {\n"
            << "    id: root\n"
            << "    anchors.fill: parent\n"
            << "    color: \"#ffffff\"\n"
            << "    Kirigami.Theme.colorSet: Kirigami.Theme.View\n"
            << "    Kirigami.Theme.inherit: false\n"
            << "    Component.onCompleted: {\n"
            << "        if (QQC2.Overlay.overlay) {\n"
            << "            QQC2.Overlay.overlay.Kirigami.Theme.colorSet = Kirigami.Theme.View\n"
            << "            QQC2.Overlay.overlay.Kirigami.Theme.inherit = false\n"
            << "        }\n"
            << "    }\n"
            << "    Loader {\n"
            << "        id: loader\n"
            << "        width: root.width\n"
            << "        height: root.height\n"
            << "        Kirigami.Theme.colorSet: Kirigami.Theme.View\n"
            << "        Kirigami.Theme.inherit: false\n"
            << "        source: \"" << mainUrl << "\"\n"
            << "        onLoaded: { if (item) { item.width = root.width; item.height = root.height } }\n"
            << "    }\n"
            << "}\n";
        wrapper.close();
    }

    QUrl reloadUrl = QUrl::fromLocalFile(wrapperPath);
    reloadUrl.setQuery(u"t=%1"_s.arg(QDateTime::currentMSecsSinceEpoch()));
    m_previewWidget->setSource(reloadUrl);
}

void WorkbenchViewSpace::openExample(const QString &pluginId)
{
    KPackage::Package pkg = KPackage::PackageLoader::self()->loadPackage(u"KirigamiWorkbench/Example"_s, pluginId);

    QString uiDir;
    if (!pkg.isValid()) {
        for (int i = 0; i < m_exampleModel->rowCount(); ++i) {
            const QModelIndex idx = m_exampleModel->index(i, 0);
            if (idx.data(ExampleModel::PluginIdRole).toString() == pluginId) {
                const QString pkgPath = idx.data(ExampleModel::PackagePathRole).toString();
                if (!pkgPath.isEmpty()) {
                    uiDir = QDir(pkgPath).filePath(u"contents/ui"_s);
                    break;
                }
            }
        }
        if (uiDir.isEmpty()) {
            qWarning() << "openExample: invalid package for pluginId:" << pluginId;
            log(u"Error: no package found for: "_s + pluginId);
            return;
        }
    } else {
        uiDir = QDir(pkg.path()).filePath(u"contents/ui"_s);
    }

    const QStringList qmlFiles = QDir(uiDir).entryList({u"*.qml"_s}, QDir::Files);

    if (qmlFiles.isEmpty()) {
        qWarning() << "openExample: no QML files in contents/ui/ for:" << pluginId;
        return;
    }

    const QString workingDir = m_project->workingDirectory();

    while (m_tabBar->count() > 0) {
        TabPage *tabPage = qobject_cast<TabPage *>(m_stack->widget(0));
        if (tabPage) {
            m_fileToTab.remove(tabPage->filePath());
            m_stack->removeWidget(tabPage);
            tabPage->deleteLater();
        }
        m_tabBar->removeTab(0);
    }

    for (const QString &fileName : qmlFiles) {
        const QString src = QDir(uiDir).filePath(fileName);
        const QString destName = (fileName.compare(u"main.qml"_s, Qt::CaseInsensitive) == 0)
            ? u"Main.qml"_s
            : fileName;
        const QString dst = QDir(workingDir).filePath(destName);
        if (QFile::exists(dst))
            QFile::remove(dst);
        if (!QFile::copy(src, dst))
            log(u"Failed to copy: "_s + QFileInfo(src).fileName());
    }

    updateFileList();

    QString mainDest;
    for (const QString &fileName : qmlFiles) {
        const QString destName = (fileName.compare(u"main.qml"_s, Qt::CaseInsensitive) == 0)
            ? u"Main.qml"_s
            : fileName;
        const QString dst = QDir(workingDir).filePath(destName);
        const QString absDst = QFileInfo(dst).absoluteFilePath();

        if (m_documents.contains(absDst)) {
            KTextEditor::Document *oldDoc = m_documents.take(absDst);
            if (oldDoc)
                oldDoc->deleteLater();
        }

        if (destName == u"Main.qml"_s) {
            mainDest = dst;
        } else {
            if (QFile::exists(dst))
                openFile(dst);
        }
    }

    if (!mainDest.isEmpty() && QFile::exists(mainDest))
        openFile(mainDest);

    reloadPreview();
}

void WorkbenchViewSpace::setGalleryVisible(bool visible)
{
    if (m_galleryPanel)
        m_galleryPanel->setVisible(visible);
}

bool WorkbenchViewSpace::isGalleryVisible() const
{
    return m_galleryPanel && m_galleryPanel->isVisible();
}

void WorkbenchViewSpace::resetToBlank()
{
    while (m_tabBar->count() > 0) {
        TabPage *tabPage = qobject_cast<TabPage *>(m_stack->widget(0));
        if (tabPage) {
            m_fileToTab.remove(tabPage->filePath());
            m_stack->removeWidget(tabPage);
            tabPage->deleteLater();
        }
        m_tabBar->removeTab(0);
    }

    for (auto *doc : std::as_const(m_documents))
        doc->deleteLater();
    m_documents.clear();

    if (m_project)
        m_project->reset();

    if (m_project && !m_project->mainComponentPath().isEmpty()) {
        updateFileList();
        openFile(m_project->mainComponentPath());
        reloadPreview();
    }
}
