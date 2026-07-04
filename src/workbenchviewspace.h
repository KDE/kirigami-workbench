// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class WorkbenchTabBar;
class ComponentProject;
class ExampleModel;
class GalleryPanel;
class TabPage;
class QGridLayout;
class QSplitter;
class QStackedWidget;
class QListWidget;
class QQuickWidget;
class QTextEdit;

namespace KTextEditor
{
class Document;
}

class WorkbenchViewSpace : public QWidget
{
    Q_OBJECT

public:
    explicit WorkbenchViewSpace(ComponentProject *project, ExampleModel *exampleModel, QWidget *parent = nullptr);
    ~WorkbenchViewSpace() override;

    TabPage *openFile(const QString &filePath);

    void closeTab(int index);
    class TabPage *currentTab() const;

    int tabCount() const;

    void focusPrevTab();
    void focusNextTab();
    void focusTab(int id);

    QVariantMap projectMapForDocument(KTextEditor::Document *doc);

    void updateFileList();
    void reloadPreview();

    void openExample(const QString &pluginId);
    void setGalleryVisible(bool visible);
    bool isGalleryVisible() const;
    void resetToBlank();

Q_SIGNALS:
    void currentTabChanged(class TabPage *tab);

private:
    void log(const QString &msg);
    void setupUI();
    void setupConnections();

    void changeTab(int idx);
    void showTabContextMenu(int tabIndex, const QPoint &globalPos);
    void showFileListContextMenu(const QPoint &pos);

    KTextEditor::Document *getOrCreateDocument(const QString &filePath);
    TabPage *createTabForFile(const QString &filePath, KTextEditor::Document *doc);

    QGridLayout *m_layout = nullptr;
    WorkbenchTabBar *m_tabBar = nullptr;
    QStackedWidget *m_stack = nullptr;

    ComponentProject *m_project = nullptr;

    ExampleModel *m_exampleModel = nullptr;

    GalleryPanel *m_galleryPanel = nullptr;
    QSplitter *m_mainSplitter = nullptr;

    QMap<QString, KTextEditor::Document *> m_documents;
    QMap<QString, TabPage *> m_fileToTab;

    QListWidget *m_fileList = nullptr;
    QQuickWidget *m_previewWidget = nullptr;
    QTextEdit *m_consoleWidget = nullptr;
};