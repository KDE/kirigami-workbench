// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <KTextEditor/MainWindow>
#include <KXmlGuiWindow>

#include <QCloseEvent>
#include <QStackedWidget>

class WorkbenchViewSpace;
class TabManager;
class ComponentProject;
class ExampleModel;
class WelcomePage;

namespace KTextEditor
{
class Application;
class View;
}

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    explicit MainWindow();

public Q_SLOTS:
    KTextEditor::MainWindow *activeMainWindow()
    {
        return m_mainWindow;
    }

    QWidget *createToolView(KTextEditor::Plugin *, QString, KTextEditor::MainWindow::ToolViewPosition, QIcon, QString)
    {
        return new QWidget();
    }

    KXMLGUIFactory *guiFactory()
    {
        return KXmlGuiWindow::guiFactory();
    }

    KTextEditor::View *activeView();

    KTextEditor::Plugin *plugin(const QString &name)
    {
        if (name == QStringLiteral("kateprojectplugin")) {
            return reinterpret_cast<KTextEditor::Plugin *>(this);
        }
        return nullptr;
    }

    void showMessage(QVariantMap map)
    {
        Q_UNUSED(map)
    }

    QVariantMap projectMapForDocument(KTextEditor::Document *doc);

    KTextEditor::Document *findUrl(const QUrl &url);

private:
    void setupActions();
    void closeEvent(QCloseEvent *event) override;
    void showWorkbench();

    ComponentProject *m_project = nullptr;
    ExampleModel *m_exampleModel = nullptr;
    WorkbenchViewSpace *m_viewSpace = nullptr;
    TabManager *m_tabManager = nullptr;
    KTextEditor::Application *m_application = nullptr;
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    QAction *m_toggleGalleryAction = nullptr;
    QStackedWidget *m_centralStack = nullptr;
    WelcomePage *m_welcomePage = nullptr;
};