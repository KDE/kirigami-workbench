// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QVariantMap>

class WorkbenchViewSpace;
class TabPage;

namespace KTextEditor
{
class Document;
}

class TabManager : public QObject
{
    Q_OBJECT

public:
    explicit TabManager(WorkbenchViewSpace *viewSpace, QObject *parent = nullptr);

    void closeTab(int index);
    TabPage *currentTab() const;
    int tabCount() const;
    QVariantMap projectMapForDocument(KTextEditor::Document *doc);

Q_SIGNALS:
    void currentTabChanged(TabPage *tab);

private:
    WorkbenchViewSpace *m_viewSpace = nullptr;
};
