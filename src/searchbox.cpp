/**************************************************************************
*   Copyright (C) 2013 by Robert Metsaranta                               *
*   therealestrob@gmail.com                                               *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/


#include <QPixmap>
#include <QDebug>
#include <QToolButton>
#include <QTimer>
#include <QMenu>
#include <QStyle>
#include <QStyleOption>

#include "searchbox.h"
#include "iconprovider.h"
#include "mainwindow.h"
#include "fsmodel.h"
#include "viewcontainer.h"

using namespace KDFM;

SearchTypeSelector::SearchTypeSelector(SearchBox *parent) : Button(parent), m_searchBox(parent)
{
    setFixedSize(16, 16);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
    QMenu *m = new QMenu(this);
    m->setSeparatorsCollapsible(false);
    m->addSeparator()->setText(tr("Searching Options:"));
    QAction *filter = m->addAction(tr("Filter quickly by name"));
    QAction *search = m->addAction(tr("Search recursively in current path"));
    m->addSeparator();
    QAction *cancel = m->addAction(tr("Cancel Search"));
    QAction *close = m->addAction(tr("Close Search"));
    setOpacity(0.5f);

    connect(filter, &QAction::triggered, this, &SearchTypeSelector::filter);
    connect(cancel, &QAction::triggered, this, &SearchTypeSelector::cancel);
    connect(close, &QAction::triggered, this, &SearchTypeSelector::closeSearch);
    setMenu(m);
    QTimer::singleShot(0, this, &SearchTypeSelector::updateIcon);
}

void
SearchTypeSelector::filter()
{
    closeSearch();
    m_searchBox->clear();
//    emit modeChanged(Filter);
}

void
SearchTypeSelector::updateIcon()
{
    setIcon(IconProvider::icon(IconProvider::Search));
}

void
SearchTypeSelector::cancel()
{
    MainWindow *mw = static_cast<MainWindow *>(window());
//    mw->currentContainer()->model()->cancelSearch();
}

void
SearchTypeSelector::closeSearch()
{
    MainWindow *mw = static_cast<MainWindow *>(window());
//    mw->currentContainer()->model()->endSearch();
}

//--------------------------------------------------------------------------

ClearSearch::ClearSearch(QWidget *parent) : Button(parent)
{
    setFixedSize(16, 16);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover);
    QStyleOption opt;
    opt.initFrom(this);
    const QIcon icon = style()->standardPixmap(QStyle::SP_LineEditClearButton, &opt, this);
    setIcon(icon);
}

//--------------------------------------------------------------------------

SearchBox::SearchBox(QWidget *parent)
    : QLineEdit(parent)
    , m_margin(4)
    , m_selector(new SearchTypeSelector(this))
    , m_clearSearch(new ClearSearch(this))
//    , m_mode(Filter)
{
    setPlaceholderText("Filter By Name");

//    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    setMaximumWidth(256);
    setMouseTracking(true);
    setAlignment(Qt::AlignCenter);
//    QFont f(font());
//    f.setBold(true);
//    setFont(f);
    connect(this, &SearchBox::textChanged, this, &SearchBox::setClearButtonEnabled);
    connect(this, &SearchBox::textChanged, this, &SearchBox::correctSelectorPos);
    connect(m_clearSearch, &ClearSearch::clicked, this, &SearchBox::clear);
    m_clearSearch->setVisible(false);
}

SearchBox::~SearchBox()
{
}

void
SearchBox::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);
//    setTextMargins(m_selector->rect().width()+m_margin, 0, m_clearSearch->width()+m_margin, 0);
//    m_selector->move(rect().left() + m_margin, (rect().bottom()-m_selector->rect().bottom())>>1);
    m_selector->move(posFor());
    m_clearSearch->move(posFor(false));
//    m_clearSearch->move(rect().right()-(m_margin+m_clearSearch->width()), (rect().bottom()-m_clearSearch->rect().bottom())>>1);
}

//void
//SearchBox::setMode(const SearchMode mode)
//{
//    if (mode==Filter)
//        setPlaceholderText("Filter by name");
//    else if (mode==Search)
//        setPlaceholderText("Search...");
//    correctSelectorPos();
//    m_mode = mode;
//}

void
SearchBox::search()
{
//    if (text().isEmpty()||m_mode == Filter)
//        return;

    FS::ProxyModel *fsModel = static_cast<MainWindow *>(window())->activeContainer()->model();
    if (!fsModel)
        return;
//    fsModel->search(text());
}

QPoint
SearchBox::posFor(bool selector) const
{
    const QRect tr(fontMetrics().boundingRect(rect(), alignment(), text().isEmpty()?placeholderText():text()));
    const QSize sz(selector?m_selector->size():m_clearSearch->size());
    const int top((size().height()/2)-(sz.height()/2));
    return QPoint(selector?tr.left()-(sz.width()+m_margin):tr.right()+m_margin, top);
}

void
SearchBox::correctSelectorPos()
{
    m_selector->move(posFor(true));
    m_clearSearch->move(posFor(false));
}
