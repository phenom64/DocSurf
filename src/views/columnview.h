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


#ifndef COLUMNVIEW_H
#define COLUMNVIEW_H

#include <QAbstractItemView>
#include <QListView>
#include <KFileItem>

class QTimer;

namespace DocSurf
{

class Grip : public QWidget
{
public:
    Grip(QWidget *parent = 0);

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);

private:
    int m_x;
};

class ColumnView;
class Column : public QListView
{
    friend class ColumnView;
    Q_OBJECT
public:
    Column(ColumnView *colView, QWidget *parent = 0);
    ~Column();
    void setRootIndex(const QModelIndex &index);

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void showEvent(QShowEvent *e);
    void resizeEvent(QResizeEvent *e);
    void paintEvent(QPaintEvent *e);
    void wheelEvent(QWheelEvent *e);
//    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

protected slots:
    void saveWidth();

signals:
    void requestRightClickMenu(const QPoint &pt);
    void newTabRequest(const QModelIndex &index);
    void opened(const QModelIndex &index);
    void expand(const QModelIndex &index);

private:
    ColumnView *m_colView;
    QModelIndex m_pressIdx;
    QPersistentModelIndex m_persistentRootIndex;
    bool m_hasBeenShown;
    int m_savedWidth;
    QTimer *m_sizeTimer;
};

class ColumnView : public QAbstractItemView
{
    Q_OBJECT
    friend class Column;
public:
    ColumnView(QWidget *parent = 0);
    ~ColumnView();

    /*reimplemented virtuals*/
    void setRootIndex(const QModelIndex &index);
    void setModel(QAbstractItemModel *model);

    /*pure virtuals*/
    QModelIndex indexAt(const QPoint &point) const;
    void scrollTo(const QModelIndex & index, ScrollHint hint = EnsureVisible);
    QRect visualRect(const QModelIndex &index) const;

    QList<QPersistentModelIndex> activeIndexes() const;
    Column *activeColumn();

public slots:
    void reset();

protected:
    /*reimplemented virtuals*/
    void resizeEvent(QResizeEvent *e);
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);

    /*pure virtuals*/
    int horizontalOffset() const;
    int verticalOffset() const;
    bool isIndexHidden(const QModelIndex &index) const;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    void setSelection(const QRect & rect, QItemSelectionModel::SelectionFlags flags);
    QRegion visualRegionForSelection(const QItemSelection &selection) const;

    void initializeColumn(Column *column);
    Column *createColumn(const QModelIndex &rootIndex);
    Column *columnForIndex(const QModelIndex &index);

protected slots:
    void open(const QModelIndex &index);
    void expand(const QModelIndex &index);
    void updateLayout();

signals:
    void newTabRequest(const QModelIndex &index);
    void opened(const QModelIndex &index);
    void requestRightClickMenu(const QPoint &pt);

private:
    QList<Column *> m_columns;
    QList<QPersistentModelIndex> m_activeIndexes;
};

}

#endif // COLUMNVIEW_H
