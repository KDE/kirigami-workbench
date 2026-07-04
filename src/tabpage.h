// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

namespace KTextEditor
{
class View;
class Document;
}

class TabPage : public QWidget
{
    Q_OBJECT

public:
    explicit TabPage(const QString &filePath, KTextEditor::Document *document, QWidget *parent = nullptr);
    ~TabPage() override;

    void save();
    bool isModified() const;

    QString filePath() const { return m_filePath; }
    KTextEditor::Document *document() const { return m_document; }
    KTextEditor::View *editorView() const { return m_editorView; }

Q_SIGNALS:
    void modifiedChanged(bool modified);

private:
    void setupUI();

    QString m_filePath;
    KTextEditor::Document *m_document = nullptr;
    KTextEditor::View *m_editorView = nullptr;
};
