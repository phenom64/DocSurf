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


#ifndef FLOW_H
#define FLOW_H

#include <QGraphicsView>

class QModelIndex;
class QItemSelectionModel;
namespace KDFM
{
namespace FS { class ProxyModel; }
class Flow : public QGraphicsView
{
    class Item;
    friend class Item;

    class RootItem;
    friend class RootItem;

    friend class GraphicsScene;
    Q_OBJECT
public:
    enum Pos { Prev = 0, New = 1 };
    explicit Flow(QWidget *parent = 0);
    ~Flow();
    void setModel(FS::ProxyModel *model);
    void setSelectionModel(QItemSelectionModel *model);
    void setCenterIndex(const QModelIndex &index);
    void showCenterIndex(const QModelIndex &index);
    void animateCenterIndex(const QModelIndex &index);
    QModelIndex indexOfItem(Item *item) const;
    bool isAnimating() const;
    float y() const;
    QList<Item *> &items() const;
    QColor &bg() const;
    
Q_SIGNALS:
    void centerIndexChanged(const QModelIndex &centerIndex);
    void opened(const QModelIndex &idx);
    
public Q_SLOTS:
    void setRootIndex(const QModelIndex &rootIndex);
    void reset();

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void enterEvent(QEvent *e);
    void prepareAnimation();
    void correctItemsPos(const int leftStart, const int rightStart);
    void showPrevious();
    void showNext();

private Q_SLOTS:
    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void rowsInserted(const QModelIndex &parent, int start, int end);
    void rowsRemoved(const QModelIndex &parent, int start, int end);
    void clear();
    void animStep(const qreal value);
    void updateItemsPos();
    void scrollBarMoved(const int value);
    void continueIf();
    void updateScene();

private:
    class Private;
    Private * const d;
    friend class Private;
};

}

#endif // FLOW_H
