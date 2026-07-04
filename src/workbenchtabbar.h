// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSet>
#include <QTabBar>
#include <QToolButton>

class WorkbenchTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit WorkbenchTabBar(QWidget *parent = nullptr);

    void readConfig();
    void setModifiedStateIcon(int idx, bool modified);

    int addTab(const QString &text);
    void removeTab(int index);

Q_SIGNALS:
    void contextMenuRequest(int id, const QPoint &globalPos);
    void newTabRequested();
    void activateViewSpaceRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *ev) override;
    void tabLayoutChange() override;
    void resizeEvent(QResizeEvent *event) override;
    QSize tabSizeHint(int index) const override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateCloseButtonPositions();
    void addCloseButton(int index);

    bool m_doubleClickNewTab = true;
    bool m_middleClickCloseTab = true;

    QList<QToolButton *> m_closeButtons;
    QSet<int> m_modifiedTabs;
};
