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


#ifndef ICONVIEW_H
#define ICONVIEW_H

#include <KF5/KItemViews/KCategorizedView>
#include "../widgets.h"

namespace DocSurf
{

class IconView : public KCategorizedView, public Configurable
{
    Q_OBJECT
public:
    explicit IconView(QWidget *parent = 0);
    ~IconView();

    void setModel(QAbstractItemModel *model);
    QModelIndex indexAt(const QPoint &point) const;

    int textLines() const;
    void reconfigure();

public slots:
    void reset();

protected:
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *even);
    void contextMenuEvent(QContextMenuEvent *e);
    void resizeEvent(QResizeEvent *e);
    void keyPressEvent(QKeyEvent *event);

protected slots:
    void updateLayout();

signals:
    void requestRightClickMenu(const QPoint &pt);
    void newTabRequest(const QModelIndex &index);
    void opened(const QModelIndex &index);

private:
    class Private;
    Private * const d;
    friend class Private;
};
}

#endif // ICONVIEW_H
