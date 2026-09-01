// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gallerypanel.h"
#include "examplemodel.h"

#include <KLocalizedString>

#include <QApplication>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

class ExampleItemDelegate : public QStyledItemDelegate
{
public:
    explicit ExampleItemDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        painter->save();

        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, option.palette.color(QPalette::AlternateBase));
        }

        const QRect r = option.rect.adjusted(8, 4, -8, -4);
        const bool selected = option.state & QStyle::State_Selected;
        const QPalette::ColorRole textRole = selected ? QPalette::HighlightedText : QPalette::Text;
        const QPalette::ColorRole dimRole = selected ? QPalette::HighlightedText : QPalette::PlaceholderText;

        QString name = index.data(Qt::DisplayRole).toString();
        QString category = index.data(ExampleModel::CategoryRole).toString();
        QString description = index.data(ExampleModel::DescriptionRole).toString();

        QFont nameFont = option.font;
        nameFont.setBold(true);
        painter->setFont(nameFont);
        painter->setPen(option.palette.color(QPalette::Active, textRole));
        painter->drawText(QRect(r.left(), r.top(), r.width(), r.height() / 2), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, name);

        const QString sub = category.isEmpty() ? description : (category + u"  ·  "_s + description);
        QFont smallFont = option.font;
        smallFont.setPointSizeF(smallFont.pointSizeF() * 0.85);
        painter->setFont(smallFont);
        painter->setPen(option.palette.color(QPalette::Active, dimRole));
        painter->drawText(QRect(r.left(), r.top() + r.height() / 2, r.width(), r.height() / 2), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, sub);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(index)
        const int lineHeight = option.fontMetrics.height();
        return QSize(option.rect.width(), qMax(48, lineHeight * 4));
    }
};

GalleryPanel::GalleryPanel(ExampleModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    setupUI();
    setupConnections();
}

void GalleryPanel::setupUI()
{
    const int margin = style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(margin / 2, margin / 2, margin / 2, margin / 2);
    layout->setSpacing(margin / 2);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText(i18nc("@info:placeholder", "Search examples\u2026"));
    m_searchBar->setClearButtonEnabled(true);
    layout->addWidget(m_searchBar);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new QListView(this);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new ExampleItemDelegate(this));
    m_listView->setUniformItemSizes(false);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(m_listView, 1);

    setMinimumWidth(160);
}

void GalleryPanel::setupConnections()
{
    connect(m_searchBar, &QLineEdit::textChanged, m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);

    connect(m_listView, &QAbstractItemView::clicked, this, [this](const QModelIndex &index) {
        const QString pluginId = index.data(ExampleModel::PluginIdRole).toString();
        if (!pluginId.isEmpty())
            Q_EMIT exampleSelected(pluginId);
    });

    m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_listView, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        const QModelIndex index = m_listView->indexAt(pos);
        if (!index.isValid())
            return;

        const QString pluginId = index.data(ExampleModel::PluginIdRole).toString();
        if (pluginId.isEmpty())
            return;

        QMenu menu(this);

        QAction *openAction = menu.addAction(QIcon::fromTheme(u"document-open-symbolic"_s), i18nc("@action:inmenu", "Open Example"));
        connect(openAction, &QAction::triggered, this, [this, pluginId]() {
            Q_EMIT exampleSelected(pluginId);
        });

        menu.addSeparator();

        QAction *createAction =
            menu.addAction(QIcon::fromTheme(u"project-development-new-template"_s), i18nc("@action:inmenu", "Create Project from Example\u2026"));
        createAction->setToolTip(i18nc("@info:tooltip", "Scaffold a new KDE project using this example's QML as the starting point."));
        connect(createAction, &QAction::triggered, this, [this, pluginId]() {
            Q_EMIT createProjectRequested(pluginId);
        });

        menu.exec(m_listView->viewport()->mapToGlobal(pos));
    });
}

#include "moc_gallerypanel.cpp"
