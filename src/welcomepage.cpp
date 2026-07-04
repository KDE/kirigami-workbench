// SPDX-FileCopyrightText: 2026 Shubham Shinde <shubshinde8381@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "welcomepage.h"
#include "examplemodel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

class WelcomeItemDelegate : public QStyledItemDelegate
{
public:
    explicit WelcomeItemDelegate(QObject *parent = nullptr)
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

        const QRect r = option.rect.adjusted(12, 4, -12, -4);
        const bool selected = option.state & QStyle::State_Selected;
        const QPalette::ColorRole textRole = selected ? QPalette::HighlightedText : QPalette::Text;
        const QPalette::ColorRole dimRole = selected ? QPalette::HighlightedText : QPalette::PlaceholderText;

        QFont bold = option.font;
        bold.setBold(true);
        painter->setFont(bold);
        painter->setPen(option.palette.color(QPalette::Active, textRole));
        painter->drawText(QRect(r.left(), r.top(), r.width(), r.height() / 2),
                          Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                          index.data(Qt::DisplayRole).toString());

        const QString sub = index.data(ExampleModel::CategoryRole).toString() + u"  ·  "_s + index.data(ExampleModel::DescriptionRole).toString();

        QFont small = option.font;
        small.setPointSizeF(small.pointSizeF() * 0.85);
        painter->setFont(small);
        painter->setPen(option.palette.color(QPalette::Active, dimRole));
        painter->drawText(QRect(r.left(), r.top() + r.height() / 2, r.width(), r.height() / 2),
                          Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                          sub);

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        const int lineHeight = option.fontMetrics.height();
        return QSize(option.rect.width(), qMax(56, lineHeight * 4));
    }
};

WelcomePage::WelcomePage(ExampleModel *model, QWidget *parent)
    : QWidget(parent)
    , m_model(model)
{
    setupUI();
    setupConnections();
}

void WelcomePage::setupUI()
{
    const int baseMargin = style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    const int largeMargin = baseMargin * 2;
    const int smallSpacing = style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    auto *header = new QWidget(this);
    header->setObjectName(u"welcomeHeader"_s);
    header->setAutoFillBackground(true);
    auto *hl = new QVBoxLayout(header);
    hl->setContentsMargins(largeMargin, largeMargin, largeMargin, largeMargin);
    hl->setSpacing(smallSpacing);

    auto *title = new QLabel(tr("Kirigami Workbench"), header);
    QFont tf = title->font();
    tf.setPointSizeF(tf.pointSizeF() * 1.8);
    tf.setBold(true);
    title->setFont(tf);

    auto *subtitle = new QLabel(tr("Prototype and explore QML components interactively."), header);
    subtitle->setWordWrap(true);

    hl->addWidget(title);
    hl->addWidget(subtitle);

    auto *scratchBtn = new QPushButton(QIcon::fromTheme(u"document-new-symbolic"_s), tr("  Start from Scratch"), header);
    scratchBtn->setObjectName(u"scratchButton"_s);
    scratchBtn->setMinimumHeight(qMax(32, scratchBtn->fontMetrics().height() * 2));
    connect(scratchBtn, &QPushButton::clicked, this, &WelcomePage::startFromScratch);
    hl->addSpacing(smallSpacing);
    hl->addWidget(scratchBtn, 0, Qt::AlignLeft);

    outerLayout->addWidget(header);

    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    outerLayout->addWidget(sep);

    auto *galleryWidget = new QWidget(this);
    auto *gl = new QVBoxLayout(galleryWidget);
    gl->setContentsMargins(largeMargin, baseMargin, largeMargin, baseMargin);
    gl->setSpacing(smallSpacing);

    auto *galleryLabel = new QLabel(tr("Or open an example to get started:"), galleryWidget);
    QFont ll = galleryLabel->font();
    ll.setBold(true);
    galleryLabel->setFont(ll);
    gl->addWidget(galleryLabel);

    m_searchBar = new QLineEdit(galleryWidget);
    m_searchBar->setPlaceholderText(tr("Search examples\u2026"));
    m_searchBar->setClearButtonEnabled(true);
    gl->addWidget(m_searchBar);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);
    m_proxyModel->setSourceModel(m_model);

    m_listView = new QListView(galleryWidget);
    m_listView->setModel(m_proxyModel);
    m_listView->setItemDelegate(new WelcomeItemDelegate(this));
    m_listView->setUniformItemSizes(true);
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setFrameShape(QFrame::NoFrame);
    gl->addWidget(m_listView, 1);

    outerLayout->addWidget(galleryWidget, 1);
}

void WelcomePage::setupConnections()
{
    connect(m_searchBar, &QLineEdit::textChanged, m_proxyModel, &QSortFilterProxyModel::setFilterFixedString);

    connect(m_listView, &QAbstractItemView::clicked, this, [this](const QModelIndex &index) {
        const QString pluginId = index.data(ExampleModel::PluginIdRole).toString();
        if (!pluginId.isEmpty())
            Q_EMIT exampleSelected(pluginId);
    });
}

#include "moc_welcomepage.cpp"
