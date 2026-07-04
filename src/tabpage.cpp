// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tabpage.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QFile>
#include <QVBoxLayout>

TabPage::TabPage(const QString &filePath, KTextEditor::Document *document, QWidget *parent)
    : QWidget(parent)
    , m_filePath(filePath)
    , m_document(document)
{
    setupUI();

    if (m_document) {
        connect(m_document, QOverload<KTextEditor::Document *>::of(&KTextEditor::Document::modifiedChanged), this, [this](KTextEditor::Document *) {
            Q_EMIT modifiedChanged(m_document->isModified());
        });
    }
}

TabPage::~TabPage() = default;

void TabPage::setupUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    if (m_document) {
        m_editorView = m_document->createView(this);
        m_editorView->setStatusBarEnabled(false);
        layout->addWidget(m_editorView);
    }
}

void TabPage::save()
{
    if (!m_document || m_filePath.isEmpty())
        return;

    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(m_document->text().toUtf8());
        file.close();
        m_document->setModified(false);
    }
}

bool TabPage::isModified() const
{
    return m_document && m_document->isModified();
}

#include "moc_tabpage.cpp"
