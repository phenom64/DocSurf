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

#include "mainwindow.h"
#include <QPainter>
#include <QTextEdit>
#include <QMessageBox>
#include <QTimer>
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QDynamicPropertyChangeEvent>
#include <QScrollBar>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QResizeEvent>

#include "columnview.h"
#include "viewcontainer.h"
#include "objects.h"

#include <fsmodel.h>

using namespace KDFM;

#define KEY "colWidth"

class ColumnDelegate : public FileItemDelegate
{
public:
    explicit ColumnDelegate(QObject *parent = 0) : FileItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        ColumnView *view = static_cast<ColumnView *>(parent());
        const bool childViews = view->activeIndexes().contains(index);
        const bool isDir = index.data(FS::DirModel::FileItemRole).value<KFileItem>().isDir();
        bool needBold(false);
        //background
        if (option.state & (QStyle::State_Selected|QStyle::State_MouseOver) || childViews)
        {
            needBold = (option.state & QStyle::State_Selected)||childViews;
            if (needBold)
                const_cast<QStyleOptionViewItem *>(&option)->state |= QStyle::State_MouseOver;
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, view);
        }

        //icon
        QStyle *style = QApplication::style();
        const QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        QPixmap pix = icon.pixmap(view->iconSize().width());
        if (pix.height() > view->iconSize().height() || pix.width() > view->iconSize().width())
            pix = pix.scaled(view->iconSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QRect ir = style->itemPixmapRect(QRect(option.rect.topLeft(), QSize(view->iconSize().width()+4, option.rect.height())), Qt::AlignCenter, pix);

        if (!pix.isNull())
            style->drawItemPixmap(painter, ir, Qt::AlignCenter, pix);

        //thumbnail shadow
//        if (index.data(FS::FileHasThumbRole).toBool())
//            drawShadow(ir.adjusted(-(shadowSize()-1), -(shadowSize()-1), shadowSize()-1, shadowSize()-1), painter);

        //text
        const QFont savedFont(painter->font());
        if (needBold)
        {
            QFont f(savedFont);
            f.setBold(true);
            painter->setFont(f);
        }
        const QRect tr(QPoint(ir.right()+4, option.rect.top()), QPoint(option.rect.right()-isDir*20, option.rect.bottom()+1));
        QApplication::style()->drawItemText(painter,
                                            tr,
                                            Qt::AlignLeft|Qt::AlignVCenter,
                                            option.palette,
                                            option.state & QStyle::State_Enabled,
                                            painter->fontMetrics().elidedText(index.data().toString(), option.textElideMode, tr.width()),
                                            option.state & QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text);
        if (needBold)
            painter->setFont(savedFont);

        //arrow
        if (isDir)
        {
            QStyleOption copy = option;
            copy.rect = copy.rect.adjusted(copy.rect.width()-20, 0, 0, 0);
            QApplication::style()->drawPrimitive(QStyle::PE_IndicatorArrowRight, &copy, painter, view);
        }
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QSize sz(FileItemDelegate::sizeHint(option, index));
        if (sz.height() < option.decorationSize.height())
            sz.setHeight(option.decorationSize.height());
        return sz;
    }
};

//---------------------------------------------------------------------------------------------------------

ColumnView::ColumnView(QWidget *parent) : QAbstractItemView(parent)
{
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
    setAcceptDrops(true);
    setDragEnabled(true);
    setEditTriggers(QAbstractItemView::SelectedClicked|QAbstractItemView::EditKeyPressed);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setTextElideMode(Qt::ElideRight);
    QAbstractItemDelegate *d = itemDelegate();
    setItemDelegate(new ColumnDelegate(this));
    delete d;
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &ColumnView::updateLayout);
}

ColumnView::~ColumnView()
{
    m_activeIndexes.clear();
}

void
ColumnView::setModel(QAbstractItemModel *model)
{
    QAbstractItemView::setModel(model);
//    connect(model, &QAbstractItemModel::layoutChanged, this, &ColumnView::reset);
//    connect(model, &QAbstractItemModel::modelReset, this, &ColumnView::reset);
}

void
ColumnView::resizeEvent(QResizeEvent *e)
{
    QAbstractItemView::resizeEvent(e);
    if (e->size().height() != e->oldSize().height())
        updateLayout();
}

void
ColumnView::setRootIndex(const QModelIndex &index)
{
    QAbstractItemView::setRootIndex(index);
    for (int i = m_columns.count()-1; i > 0; --i)
    {
        m_columns.at(i)->hide();
        m_columns.takeAt(i)->deleteLater();
    }
    if (!m_columns.count())
        m_columns << createColumn(index);
    else
        m_columns.at(0)->setRootIndex(index);
    updateLayout();
}

void
ColumnView::reset()
{
    for (int i = m_columns.count()-1; i > 0; --i)
        m_columns.takeAt(i)->deleteLater();
//    destroyColumns(true);
//    m_columns << createColumn(rootIndex());
}

QList<QPersistentModelIndex>
ColumnView::activeIndexes() const
{
    return m_activeIndexes;
//    if (m_columns.isEmpty())
//        return QList<QModelIndex>();
//    QList<QModelIndex> list;
//    QModelIndex idx = m_columns.last()->rootIndex();
//    while (idx.isValid())
//    {
//        list.insert(0, idx);
//        idx = idx.parent();
//    }
//    return list;
}

void
ColumnView::open(const QModelIndex &index)
{
    emit opened(index);
}

void
ColumnView::expand(const QModelIndex &index)
{
    Column *column = static_cast<Column *>(sender());
    for (int i = m_columns.count()-1; i > m_columns.indexOf(column); --i)
    {
        m_columns.at(i)->hide();
        m_columns.takeAt(i)->deleteLater();
    }
    m_columns << createColumn(index);
    clearSelection();
    updateLayout();
    if (horizontalScrollBar()->value() < horizontalScrollBar()->maximum())
        horizontalScrollBar()->setValue(horizontalScrollBar()->maximum());
}

Column
*ColumnView::columnForIndex(const QModelIndex &index)
{
    for (int i = 0; i < m_columns.count(); ++i)
        if (m_columns.at(i)->rootIndex() == index)
            return m_columns.at(i);
    return 0;
}

Column
*ColumnView::createColumn(const QModelIndex &rootIndex)
{
    m_activeIndexes << rootIndex;
    Column *column = new Column(this, viewport());
    initializeColumn(column);
    column->setRootIndex(rootIndex);
    connect(column, &Column::opened, this, &ColumnView::open);
    connect(column, &Column::newTabRequest, this, &ColumnView::newTabRequest);
    connect(column, &Column::entered, this, &ColumnView::entered);
    connect(column, &Column::expand, this, &ColumnView::expand);
    connect(column, &Column::requestRightClickMenu, this, &ColumnView::requestRightClickMenu);
    column->show();
    return column;
}

void
ColumnView::initializeColumn(Column *column)
{
    column->setIconSize(iconSize());
    column->setFrameShape(QFrame::NoFrame);
    column->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    column->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    column->setMinimumWidth(100);
    column->setAttribute(Qt::WA_MacShowFocusRect, false);
    column->setDragDropMode(dragDropMode());
    column->setDragDropOverwriteMode(dragDropOverwriteMode());
    column->setDropIndicatorShown(showDropIndicator());
    column->setAlternatingRowColors(alternatingRowColors());
    column->setAutoScroll(hasAutoScroll());
    column->setEditTriggers(editTriggers());
    column->setHorizontalScrollMode(horizontalScrollMode());
    column->setIconSize(iconSize());
    column->setSelectionBehavior(selectionBehavior());
    column->setSelectionMode(selectionMode());
    column->setTabKeyNavigation(tabKeyNavigation());
    column->setTextElideMode(textElideMode());
    column->setVerticalScrollMode(verticalScrollMode());
    column->setModel(model());
    column->setSelectionModel(selectionModel());
    column->setCornerWidget(new Grip(column));
    connect(column, &Column::viewportEntered, this, &ColumnView::viewportEntered);

    // set the delegate to be the columnview delegate
    QAbstractItemDelegate *delegate = column->itemDelegate();
    column->setItemDelegate(itemDelegate());
    delete delegate;
}

void
ColumnView::updateLayout()
{
    int x = 0;
    for (int i = 0; i < m_columns.count(); ++i)
    {
        Column *c = m_columns.at(i);
        c->move(x-horizontalScrollBar()->value(), 0);
        c->resize(c->width(), viewport()->height());
        x += c->width();
    }
    horizontalScrollBar()->setPageStep(viewport()->width());
    if (x > viewport()->width())
        horizontalScrollBar()->setRange(0, x-viewport()->width());
    else
        horizontalScrollBar()->setRange(0, -1);
    for (int i = 0; i < m_columns.count(); ++i)
        m_columns.at(i)->update();
}

Column
*ColumnView::activeColumn()
{
    for (int i = 0; i < m_columns.count(); ++i)
        if (m_columns.at(i)->hasFocus())
            return m_columns.at(i);
    if (m_columns.count())
        return m_columns.at(0); //hmmm
    return 0;
}

void
ColumnView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
    QList<QPersistentModelIndex> active = activeIndexes();
    QModelIndexList removedIndexes;
    for (int i = start; i <= end; ++i)
    {
        const QModelIndex &index = model()->index(i, 0, parent);
        if (active.contains(index))
            removedIndexes << index;
    }
    if (removedIndexes.isEmpty())
        return;

    for (int i = m_columns.count()-1; i > -1; --i)
    {
        Column *c = m_columns.at(i);
        if (removedIndexes.contains(c->rootIndex()))
        {
            m_activeIndexes.removeOne(c->rootIndex());
            c->hide();
            m_columns.removeOne(c);
            c->deleteLater();
        }
    }
    updateLayout();
}

bool
ColumnView::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event)
{
    for (int i = 0; i < m_columns.count(); ++i)
    {
        Column *c = m_columns.at(i);
        if (index.parent() == c->rootIndex())
            return c->edit(index, trigger, event);
    }
    return false;
}

/*
 * Pure virtual methods
 */

QModelIndex
ColumnView::indexAt(const QPoint &point) const
{
    for (int i = 0; i < m_columns.count(); ++i)
    {
        Column *c = m_columns.at(i);
        if (c->geometry().contains(point))
            return c->indexAt(point+c->frameGeometry().topLeft());
    }
    return QModelIndex();
}

void
ColumnView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
    //hmm
}

QRect
ColumnView::visualRect(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRect();
    for (int i = 0; i < m_columns.count(); ++i)
    {
        const Column *const c = m_columns.at(i);
        QRect r = c->visualRect(index);
        if (r.isValid())
        {
            r.translate(c->frameGeometry().topLeft());
            return r;
        }
    }
    return QRect();
}

int
ColumnView::horizontalOffset() const
{
    return horizontalScrollBar()->value();
}

int
ColumnView::verticalOffset() const
{
    return 0;
}

bool
ColumnView::isIndexHidden(const QModelIndex &index) const
{
    return false;
}

void
ColumnView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags flags)
{

}

QModelIndex
ColumnView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    QModelIndex current = currentIndex();
    if (isRightToLeft()) {
        if (cursorAction == MoveLeft)
            cursorAction = MoveRight;
        else if (cursorAction == MoveRight)
            cursorAction = MoveLeft;
    }
    switch (cursorAction) {
    case MoveLeft:
        if (current.parent().isValid() && current.parent() != rootIndex())
            return (current.parent());
        else
            return current;
        break;

    case MoveRight:
        if (model()->hasChildren(current))
            return model()->index(0, 0, current);
        else
            return current.sibling(current.row() + 1, current.column());
        break;

    default:
        break;
    }

    return QModelIndex();
}

QRegion
ColumnView::visualRegionForSelection(const QItemSelection &selection) const
{
    return QRegion();
}

/*
 * End pure virtual methods
 */

//---------------------------------------------------------------------------------------------------------

Column::Column(ColumnView *colView, QWidget *parent)
    : QListView(parent)
    , m_hasBeenShown(false)
    , m_sizeTimer(new QTimer(this))
    , m_colView(colView)
{
    setSelectionRectVisible(true);
    setUniformItemSizes(true);
    ScrollAnimator::manage(this);
    connect(m_sizeTimer, &QTimer::timeout, this, &Column::saveWidth);
}

Column::~Column()
{
    if (m_colView->m_activeIndexes.contains(rootIndex()))
        m_colView->m_activeIndexes.removeOne(rootIndex());
}

void
Column::paintEvent(QPaintEvent *e)
{
    QPainter p(viewport());
    for (int i = 0; i < model()->rowCount(rootIndex()); ++i)
    {
        const QModelIndex &index = model()->index(i, 0, rootIndex());
        QStyleOptionViewItemV4 opt = viewOptions();
        opt.rect = visualRect(index);
        if (!opt.rect.intersects(viewport()->rect()))
            continue;
        if (opt.rect.width() > viewport()->width())
            opt.rect.setWidth(viewport()->width());
        if (opt.rect.contains(viewport()->mapFromGlobal(QCursor::pos())))
            opt.state |= QStyle::State_MouseOver;
        if (selectionModel()->isSelected(index))
            opt.state |= QStyle::State_Selected;
        itemDelegate()->paint(&p, opt, index);
    }
    p.end();
    e->accept();
}

void
Column::setRootIndex(const QModelIndex &index)
{
    QListView::setRootIndex(index);
    m_persistentRootIndex = index;
}

void
Column::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        clearSelection();
    if (e->key() == Qt::Key_Return
         && e->modifiers() == Qt::NoModifier
         && state() == NoState)
    {
        if (selectionModel()->selectedIndexes().count())
            foreach (const QModelIndex &index, selectionModel()->selectedIndexes())
                if (!index.column())
                    emit opened(index);
        e->accept();
        return;
    }
    if (e->key() == Qt::Key_Meta)
        setDragEnabled(false);
    QListView::keyPressEvent(e);
//    DViewBase::keyPressEvent(e);
}

void
Column::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Meta)
        setDragEnabled(true);
    QListView::keyReleaseEvent(e);
}

void
Column::mousePressEvent(QMouseEvent *e)
{
    m_pressIdx = indexAt(e->pos());
    QListView::mousePressEvent(e);
}

void
Column::mouseReleaseEvent(QMouseEvent *e)
{
    if (m_pressIdx.isValid() && m_pressIdx == indexAt(e->pos()))
    {
        e->accept();
        if (e->button() == Qt::MiddleButton)
        {
            emit newTabRequest(m_pressIdx);
            m_pressIdx = QModelIndex();
            return;
        }
        if (false/*Store::config.views.singleClick*/
                && e->button() == Qt::LeftButton
                && !e->modifiers())
        {
            if (m_pressIdx.data(FS::DirModel::FileItemRole).value<KFileItem>().isDir())
                emit expand(m_pressIdx);
            else
                emit opened(m_pressIdx);
            m_pressIdx = QModelIndex();
            return;
        }
    }
    QListView::mouseReleaseEvent(e);
}

void
Column::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (m_pressIdx.isValid() && m_pressIdx == indexAt(e->pos()))
    {
        e->accept();
        if (true /*Store::config.views.singleClick*/)
        if (e->button() == Qt::LeftButton)
        {
            if (m_pressIdx.data(FS::DirModel::FileItemRole).value<KFileItem>().isDir())
                emit expand(m_pressIdx);
            else
                emit opened(m_pressIdx);
            m_pressIdx = QModelIndex();
            return;
        }
    }
    QListView::mouseDoubleClickEvent(e);
}

void
Column::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::CTRL)
    {
        MainWindow *mw = static_cast<MainWindow *>(window());
        if (QSlider *s = mw->iconSizeSlider())
            s->setValue(s->value()+(e->delta()>0?1:-1));
    }
    else
        QListView::wheelEvent(e);
    e->accept();
}

void
Column::showEvent(QShowEvent *e)
{
    QListView::showEvent(e);
//    if (!m_hasBeenShown)
//    {
//        bool ok;
//        int width = Ops::getDesktopValue<int>(QDir(rootIndex().data(FS::FilePathRole).toString()), KEY, &ok, rootIndex().data(FS::UrlRole).toUrl().toString());
//        if (!ok)
//            width = Store::config.views.columnsView.colWidth;
//        resize(width, height());
//        m_hasBeenShown = true;
//    }
}

void
Column::resizeEvent(QResizeEvent *e)
{
    QListView::resizeEvent(e);
    if (e->size().width() != e->oldSize().width())
    {
        static_cast<ColumnView *>(parent()->parent())->updateLayout();
        if (!m_sizeTimer->isActive())
            m_sizeTimer->start(250);
    }
}

void
Column::contextMenuEvent(QContextMenuEvent *e)
{
    e->accept();
    emit requestRightClickMenu(e->globalPos());
}

void
Column::saveWidth()
{
//    if (Store::config.views.dirSettings && m_hasBeenShown && m_savedWidth == width())
//    {
//        Ops::writeDesktopValue<int>(QDir(rootIndex().data(FS::FilePathRole).toString()), KEY, size().width(), rootIndex().data(FS::UrlRole).toUrl().toString());
//        m_savedWidth = -1;
//        m_sizeTimer->stop();
//        return;
//    }
//    m_savedWidth = width();
}

//---------------------------------------------------------------------------------------------------------

Grip::Grip(QWidget *parent)
    : QWidget(parent)
    , m_x(-1)
{
    setCursor(Qt::SplitHCursor);
}

void
Grip::mousePressEvent(QMouseEvent *e)
{
    m_x = e->globalX();
    e->accept();
}

void
Grip::mouseReleaseEvent(QMouseEvent *e)
{
    m_x = -1;
    e->accept();
}

void
Grip::mouseMoveEvent(QMouseEvent *e)
{
    if (m_x != -1)
    {
        const QSize size = parentWidget()->size();
        parentWidget()->resize(parentWidget()->width() + (e->globalX()-m_x), parentWidget()->height());
        if (size != parentWidget()->size())
            m_x = e->globalX();
    }
    e->accept();
}

void
Grip::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawControl(QStyle::CE_ColumnViewGrip, &opt, &painter, this);
    e->accept();
}
