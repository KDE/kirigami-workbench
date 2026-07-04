// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class ExampleModel;
class QLineEdit;
class QListView;
class QSortFilterProxyModel;
class QStyledItemDelegate;

class GalleryPanel : public QWidget
{
    Q_OBJECT
public:
    explicit GalleryPanel(ExampleModel *model, QWidget *parent = nullptr);

Q_SIGNALS:
    void exampleSelected(const QString &pluginId);

private:
    void setupUI();
    void setupConnections();

    QLineEdit *m_searchBar = nullptr;
    QListView *m_listView = nullptr;
    QSortFilterProxyModel *m_proxyModel = nullptr;
    ExampleModel *m_model = nullptr;
};
