// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "workbenchtabbar.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QApplication>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOptionTab>

using namespace Qt::StringLiterals;

static int closeBtnSize()
{
    return qMax(14, qApp->style()->pixelMetric(QStyle::PM_SmallIconSize) - 2);
}
static int closeBtnMargin()
{
    return qMax(3, qApp->style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing) / 4);
}
static int tabHeight()
{
    return qMax(26, QFontMetrics(qApp->font()).height() + 10);
}

WorkbenchTabBar::WorkbenchTabBar(QWidget *parent)
    : QTabBar(parent)
{
    setDocumentMode(true);
    setMovable(true);
    setTabsClosable(false);
    setElideMode(Qt::ElideMiddle);
    setExpanding(false);
    readConfig();
    connect(this, &QTabBar::currentChanged, this, &WorkbenchTabBar::updateCloseButtonPositions);
}

int WorkbenchTabBar::addTab(const QString &text)
{
    int idx = QTabBar::addTab(text);
    addCloseButton(idx);
    updateCloseButtonPositions();
    return idx;
}

void WorkbenchTabBar::removeTab(int index)
{
    if (index >= 0 && index < m_closeButtons.size()) {
        m_closeButtons[index]->deleteLater();
        m_closeButtons.removeAt(index);
    }
    QSet<int> updated;
    for (int idx : std::as_const(m_modifiedTabs)) {
        if (idx < index)
            updated.insert(idx);
        else if (idx > index)
            updated.insert(idx - 1);
    }
    m_modifiedTabs = updated;
    QTabBar::removeTab(index);
    updateCloseButtonPositions();
}

void WorkbenchTabBar::readConfig()
{
    KConfigGroup config = KSharedConfig::openConfig()->group(u"WorkbenchTabBar"_s);
    m_doubleClickNewTab = config.readEntry("DoubleClickNewTab", true);
    m_middleClickCloseTab = config.readEntry("MiddleClickCloseTab", true);
    setExpanding(config.readEntry("ExpandTabs", false));

    const QString elideMode = config.readEntry("ElideMode", u"ElideMiddle"_s);
    if (elideMode == u"ElideLeft"_s)
        setElideMode(Qt::ElideLeft);
    else if (elideMode == u"ElideRight"_s)
        setElideMode(Qt::ElideRight);
    else if (elideMode == u"ElideNone"_s)
        setElideMode(Qt::ElideNone);
    else
        setElideMode(Qt::ElideMiddle);
}

void WorkbenchTabBar::setModifiedStateIcon(int idx, bool modified)
{
    if (idx < 0 || idx >= count())
        return;
    if (modified)
        m_modifiedTabs.insert(idx);
    else
        m_modifiedTabs.remove(idx);
    update();
}

QSize WorkbenchTabBar::tabSizeHint(int index) const
{
    const int btnSize = closeBtnSize();
    const int btnMargin = closeBtnMargin();
    QSize s = QTabBar::tabSizeHint(index);
    s.setHeight(tabHeight());
    int w = s.width() + btnSize + btnMargin * 2;
    s.setWidth(qBound(90, w, 200));
    return s;
}

void WorkbenchTabBar::tabLayoutChange()
{
    QTabBar::tabLayoutChange();
    updateCloseButtonPositions();
}

void WorkbenchTabBar::resizeEvent(QResizeEvent *event)
{
    QTabBar::resizeEvent(event);
    updateCloseButtonPositions();
}

void WorkbenchTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (m_doubleClickNewTab && event->button() == Qt::LeftButton && tabAt(event->pos()) < 0) {
        Q_EMIT newTabRequested();
        event->accept();
        return;
    }
    QTabBar::mouseDoubleClickEvent(event);
}

void WorkbenchTabBar::mousePressEvent(QMouseEvent *event)
{
    if (m_middleClickCloseTab && event->button() == Qt::MiddleButton) {
        const int idx = tabAt(event->pos());
        if (idx >= 0) {
            Q_EMIT tabCloseRequested(idx);
            event->accept();
            return;
        }
    }
    if (event->button() == Qt::LeftButton)
        Q_EMIT activateViewSpaceRequested();
    QTabBar::mousePressEvent(event);
}

void WorkbenchTabBar::contextMenuEvent(QContextMenuEvent *ev)
{
    const int idx = tabAt(ev->pos());
    if (idx >= 0) {
        Q_EMIT contextMenuRequest(idx, ev->globalPos());
        ev->accept();
    } else {
        QTabBar::contextMenuEvent(ev);
    }
}

void WorkbenchTabBar::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    QStyleOptionTab opt;

    for (int i = 0; i < count(); ++i) {
        initStyleOption(&opt, i);
        opt.text = QString();
        opt.icon = QIcon();
        style()->drawControl(QStyle::CE_TabBarTab, &opt, &p, this);

        QRect r = tabRect(i);
        const bool modified = m_modifiedTabs.contains(i);
        const int btnSize = closeBtnSize();
        const int btnMargin = closeBtnMargin();
        const int rightEdge = r.right() - btnSize - btnMargin * 2 - 4;

        constexpr int DOT_W = 10;
        int textLeft = r.left() + 8;
        if (modified) {
            QRect dotRect(textLeft, r.top(), DOT_W, r.height());
            p.setPen(opt.palette.color(QPalette::Active, QPalette::Highlight));
            p.drawText(dotRect, Qt::AlignLeft | Qt::AlignVCenter, QString(QChar(0x25CF)));
            textLeft += DOT_W + 2;
        }

        QRect textRect(textLeft, r.top(), rightEdge - textLeft, r.height());
        QColor textColor = opt.palette.color(i == currentIndex() ? QPalette::Active : QPalette::Disabled, QPalette::ButtonText);
        p.setPen(textColor);
        p.setFont(font());

        QString text = tabText(i);
        text.remove(QLatin1Char('&'));
        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);
    }
}

void WorkbenchTabBar::addCloseButton(int index)
{
    auto *btn = new QToolButton(this);
    btn->setAutoRaise(true);
    btn->setFixedSize(closeBtnSize(), closeBtnSize());
    btn->setIcon(QIcon::fromTheme(u"tab-close-symbolic"_s));
    btn->setToolTip(tr("Close tab"));
    btn->setCursor(Qt::ArrowCursor);

    connect(btn, &QToolButton::clicked, this, [this, btn]() {
        const int idx = m_closeButtons.indexOf(btn);
        if (idx >= 0)
            Q_EMIT tabCloseRequested(idx);
    });

    if (index >= m_closeButtons.size())
        m_closeButtons.append(btn);
    else
        m_closeButtons.insert(index, btn);

    btn->show();
}

void WorkbenchTabBar::updateCloseButtonPositions()
{
    const QPalette &pal = palette();
    const QColor highlightColor = pal.color(QPalette::Highlight);
    const QString activeStyle = u"QToolButton { border:none; border-radius:2px; background-color:%1; padding:0px; }"
                                u"QToolButton:pressed { background-color:%2; }"_s
                                    .arg(highlightColor.name(), highlightColor.darker(120).name());
    const QString inactiveStyle = u"QToolButton { border:none; border-radius:2px; background-color:%1; padding:0px; }"
                                  u"QToolButton:hover { background-color:%2; }"
                                  u"QToolButton:pressed { background-color:%3; }"_s
                                      .arg(pal.color(QPalette::Button).name(), highlightColor.name(), highlightColor.darker(120).name());

    for (int i = 0; i < m_closeButtons.size() && i < count(); ++i) {
        const int btnSize = closeBtnSize();
        const int btnMargin = closeBtnMargin();
        QRect r = tabRect(i);
        int x = r.right() - btnSize - btnMargin;
        int y = r.top() + (r.height() - btnSize) / 2;
        m_closeButtons[i]->setFixedSize(btnSize, btnSize);
        m_closeButtons[i]->move(x, y);
        m_closeButtons[i]->raise();
        m_closeButtons[i]->setStyleSheet(i == currentIndex() ? activeStyle : inactiveStyle);
    }
}

#include "moc_workbenchtabbar.cpp"
