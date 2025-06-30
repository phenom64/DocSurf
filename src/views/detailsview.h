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


#ifndef DETAILSVIEW_H
#define DETAILSVIEW_H

#include <QTreeView>

namespace KDFM
{
class ViewContainer;
class DetailsView : public QTreeView
{
    friend class FlowView;
    Q_OBJECT
public:
    explicit DetailsView(QWidget *parent = 0);
    ~DetailsView();
    void setModel(QAbstractItemModel *model);
    ViewContainer *container();

public slots:
    inline void edit(const QModelIndex &index) { QTreeView::edit(index); }

protected:
    void contextMenuEvent(QContextMenuEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *);
    void wheelEvent(QWheelEvent *e);
    bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event);

signals:
    void requestRightClickMenu(const QPoint &pt);
    void newTabRequest(const QModelIndex &path);
    void opened(const QModelIndex &index);

private slots:
    void readSettings();

private:
    QPoint m_pressPos;
    void *m_pressedIndex;
};

}

#endif // DETAILSVIEW_H
