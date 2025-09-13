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

#include <QMenu>
#include <QDragMoveEvent>
#include <QDebug>
#include <QHeaderView>
#include <qmath.h>
#include <QTextEdit>
#include <QMessageBox>
#include <QPainter>
#include <KDirSortFilterProxyModel>
#include "viewcontainer.h"
#include "detailsview.h"
#include "mainwindow.h"
#include "objects.h"

using namespace DocSurf;

class DetailsDelegate : public FileItemDelegate
{
public:
    DetailsDelegate(QObject *parent) : FileItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if (!index.isValid())
            return;

        const bool isHovered = option.state & QStyle::State_MouseOver;
        const bool isSelected = option.state & QStyle::State_Selected;
        if (index.column() > 0 &&  !isHovered && !isSelected)
        {
            QColor c = option.palette.color(QPalette::Text);
            c.setAlpha(127);
            const_cast<QStyleOptionViewItem *>(&option)->palette.setColor(QPalette::Text, c);
        }

        FileItemDelegate::paint(painter, option, index);
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return FileItemDelegate::sizeHint(option, index) /*+ QSize(0, Store::config.views.detailsView.rowPadding*2)*/;
    }
};

DetailsView::DetailsView(QWidget *parent)
    : QTreeView(parent)
    , m_pressPos(QPoint())
    , m_pressedIndex(0)
{
    ScrollAnimator::manage(this);
    itemDelegate()->deleteLater();
    setItemDelegate(new DetailsDelegate(this));
    header()->setStretchLastSection(false);
    setUniformRowHeights(true);
    setSelectionMode(ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSortingEnabled(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers); // Disable click editing
    setExpandsOnDoubleClick(false);
    setDragDropMode(DragDrop);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setAcceptDrops(true);
    setDragEnabled(true);
    setVerticalScrollMode(ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlternatingRowColors(false/*Store::config.views.detailsView.altRows*/);
    header()->setSortIndicator(0, Qt::AscendingOrder);
}

DetailsView::~DetailsView()
{

}

void
DetailsView::readSettings()
{
    setAlternatingRowColors(false/*Store::config.views.detailsView.altRows*/);
}

ViewContainer
*DetailsView::container()
{
    QWidget *w = this;
    while (w)
    {
        if (ViewContainer *c = qobject_cast<ViewContainer *>(w))
            return c;
        w = w->parentWidget();
    }
    return 0;
}

void
DetailsView::contextMenuEvent(QContextMenuEvent *e)
{
    e->accept();
    emit requestRightClickMenu(e->globalPos());
}

void
DetailsView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QTreeView::mouseDoubleClickEvent(event);
    const QModelIndex &index = indexAt(event->pos());
    if (index.isValid() && event->button() == Qt::LeftButton && !event->modifiers() && state() == NoState)
        emit opened(index);
}

void
DetailsView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    for (int i = 0; i < header()->count(); ++i)
    {
        header()->setSectionHidden(i, i > 3);
        header()->setSectionResizeMode(i, i == 0 ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
    }
    if (KDirSortFilterProxyModel *sfpModel = qobject_cast<KDirSortFilterProxyModel *>(model))
    {
        header()->setSortIndicator(sfpModel->sortColumn(), sfpModel->sortOrder());
        connect(sfpModel, &KDirSortFilterProxyModel::layoutChanged, this, [this]()
        {
            KDirSortFilterProxyModel *m = static_cast<KDirSortFilterProxyModel *>(sender());
            header()->setSortIndicator(m->sortColumn(), m->sortOrder());
        });
    }
}

void
DetailsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
        clearSelection();
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && state() == NoState)
    {
        if (currentIndex().isValid()) {
            edit(currentIndex()); // Rename on Enter
            event->accept();
            return;
        }
    }
    QTreeView::keyPressEvent(event);
}

void
DetailsView::mouseReleaseEvent(QMouseEvent *e)
{
    setDragEnabled(true);
    const QModelIndex &index = indexAt(e->pos());

    if (!index.isValid())
    {
        QTreeView::mouseReleaseEvent(e);
        return;
    }

    if (false/*Store::config.views.singleClick*/
            && !e->modifiers()
            && e->button() == Qt::LeftButton
            && m_pressedIndex == index.internalPointer()
            && visualRect(index).contains(e->pos()) //clicking on expanders should not open the dir...
            && !state()
            && !index.column())
    {
        emit opened(index);
        e->accept();
        return;
    }

    if (e->button() == Qt::MiddleButton
            && m_pressPos == e->pos()
            && !e->modifiers())
    {
        e->accept();
        emit newTabRequest(index);
        return;
    }

    QTreeView::mouseReleaseEvent(e);
}

void
DetailsView::mousePressEvent(QMouseEvent *event)
{
    if (event->modifiers() & Qt::MetaModifier || indexAt(event->pos()).column())
        setDragEnabled(false);
    m_pressedIndex = indexAt(event->pos()).internalPointer();
    m_pressPos = event->pos();
    QTreeView::mousePressEvent(event);
}

void
DetailsView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::CTRL)
    {
        MainWindow *mw = static_cast<MainWindow *>(window());
        if (QSlider *s = mw->iconSizeSlider())
            s->setValue(s->value() + (e->angleDelta().y() > 0 ? 1 : -1));
    }
    else
        QTreeView::wheelEvent(e);
    e->accept();
}

bool
DetailsView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event)
{
    if (!index.isValid())
        return false;
    QModelIndex idx = index;
    if (idx.column())
        idx = idx.sibling(idx.row(), 0);
    return QTreeView::edit(idx, trigger, event);
}
