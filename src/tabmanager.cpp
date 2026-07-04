// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tabmanager.h"
#include "tabpage.h"
#include "workbenchviewspace.h"

#include <KTextEditor/Document>

TabManager::TabManager(WorkbenchViewSpace *viewSpace, QObject *parent)
    : QObject(parent)
    , m_viewSpace(viewSpace)
{
    connect(m_viewSpace, &WorkbenchViewSpace::currentTabChanged, this, &TabManager::currentTabChanged);
}

void TabManager::closeTab(int index)
{
    m_viewSpace->closeTab(index);
}

TabPage *TabManager::currentTab() const
{
    return m_viewSpace->currentTab();
}

int TabManager::tabCount() const
{
    return m_viewSpace->tabCount();
}

QVariantMap TabManager::projectMapForDocument(KTextEditor::Document *doc)
{
    return m_viewSpace->projectMapForDocument(doc);
}

#include "moc_tabmanager.cpp"
