// SPDX-FileCopyrightText: 2024 Joshua Goins <josh@redstrate.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <KTextEditor/MainWindow>
#include <KXmlGuiWindow>

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QQuickWidget>
#include <QTemporaryFile>
#include <QVBoxLayout>

namespace KTextEditor
{
class Application;
}

class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT
public:
    explicit MainWindow();

public Q_SLOTS:
    // These slots implements the plugin interface in Kate we need for the LSP plugin
    KTextEditor::MainWindow *activeMainWindow()
    {
        return m_mainWindow;
    }

    QWidget *createToolView(KTextEditor::Plugin *, QString, KTextEditor::MainWindow::ToolViewPosition, QIcon, QString)
    {
        auto widget = new QWidget();
        auto layout = new QVBoxLayout();
        widget->setLayout(layout);

        return widget;
    }

    KXMLGUIFactory *guiFactory()
    {
        return KXmlGuiWindow::guiFactory();
    }

    KTextEditor::View *activeView()
    {
        return m_view;
    }

    KTextEditor::Plugin *plugin(const QString &name)
    {
        if (name == QStringLiteral("kateprojectplugin")) {
            return reinterpret_cast<KTextEditor::Plugin *>(this);
        }
        return nullptr;
    }

    void showMessage(QVariantMap map)
    {
        // TODO: Implement?
        Q_UNUSED(map)
    }

    QVariantMap projectMapForDocument(KTextEditor::Document *)
    {
        const QFileInfo info(qmlWorkFile.fileName());
        return QJsonObject{{QStringLiteral("lspclient"),
                            QJsonObject{{QStringLiteral("servers"),
                                         QJsonObject{{QStringLiteral("qml"), QJsonObject{{QStringLiteral("root"), info.absoluteDir().absolutePath()}}}}}}}}
            .toVariantMap();
    }

    KTextEditor::Document *findUrl(const QUrl &url)
    {
        Q_UNUSED(url)
        return m_doc;
    }

private:
    QQuickWidget *m_quickWidget = nullptr;
    KTextEditor::Application *m_application = nullptr;
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    KTextEditor::View *m_view = nullptr;
    QTemporaryFile qmlWorkFile;
    KTextEditor::Document *m_doc = nullptr;
};