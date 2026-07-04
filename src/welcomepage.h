// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class ExampleModel;
class QLineEdit;
class QListView;
class QSortFilterProxyModel;

class WelcomePage : public QWidget
{
    Q_OBJECT
public:
    explicit WelcomePage(ExampleModel *model, QWidget *parent = nullptr);

Q_SIGNALS:
    void startFromScratch();
    void exampleSelected(const QString &pluginId);

private:
    void setupUI();
    void setupConnections();

    ExampleModel *m_model = nullptr;
    QLineEdit *m_searchBar = nullptr;
    QListView *m_listView = nullptr;
    QSortFilterProxyModel *m_proxyModel = nullptr;
};
