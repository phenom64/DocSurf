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


#ifndef VIEWCONTAINER_H
#define VIEWCONTAINER_H

#include <QWidget>
#include <QUrl>
#include <QDir>
#include "widgets.h"

class KFileItem;
class KFileItemList;
class KUrlNavigator;
class QItemSelectionModel;
class QModelIndex;
class QAbstractItemView;
class KFilePlacesModel;
typedef QList<QModelIndex> QModelIndexList;
namespace DocSurf
{
class FlowView;
class ColumnView;
class DetailsView;
class IconView;
namespace FS{class ProxyModel;}

class ViewContainer : public QWidget, public Configurable
{
    Q_OBJECT
public:
    enum View { Icon = 0, Details, Column, Flow, NViews };
    static quint8 viewAction(const View view);
    quint8 currentViewAction() const;
    static QString prettySize(quint64 bytes);
    explicit ViewContainer(QWidget *parent = 0, KFilePlacesModel *placesModel = 0, const QUrl &url = QUrl::fromLocalFile(QDir::homePath()));
    ~ViewContainer();
    FS::ProxyModel *model();
    void setView(const View view, bool store = true);
    QAbstractItemView *currentView() const;
    View currentViewType() const;
    QString title() const;
    void setPathEditable(const bool editable);
    void setRootUrl(const QUrl &url);
    QUrl rootUrl() const;
    void setFilter(const QString &filter);
    void deleteCurrentSelection();
    const QSize iconSize() const;
    QItemSelectionModel *selectionModel() const;
    QModelIndex indexAt(const QPoint &p) const;
    QModelIndexList selectedItems() const;
    QList<QUrl> selectedUrls() const;
    KFileItemList selectedFiles() const;
    bool canGoBack();
    bool canGoForward();
    bool pathVisible();
    void goBack();
    void goForward();
    bool goUp();
    void refresh();
    void rename();
    void goHome();
    void setIconSize(int stop);
    KUrlNavigator *urlNav();
    void restoreFromTrash() const;
    void moveToTrash() const;
    const KFileItem &rootItem() const;
    void sort(const int column = 0, const Qt::SortOrder order = Qt::AscendingOrder);
#define D_VIEW(_TYPE_, _METHOD_) _TYPE_##View *_METHOD_##View()
    D_VIEW(Icon, icon); D_VIEW(Details, details); D_VIEW(Column, column); D_VIEW(Flow, flow);
#undef D_VIEW

    void reconfigure();

protected:
    void setRootIndex(const QModelIndex &index);

Q_SIGNALS:
    void urlChanged(const QUrl &url);
    void urlLoaded(const QUrl &url);
    void urlItemsChanged();
    void viewChanged();
    void iconSizeChanged(const QSize &size);
    void itemHovered(const QString &index);
    void clearHovered();
    void newTabRequest(const QUrl &url);
    void entered(const QModelIndex &index);
    void selectionChanged();

public Q_SLOTS:
    void activate(const QModelIndex &index);
    void setPathVisible(bool visible);
    void slotMakeRightClickMenu(const QPoint &pt);
    void slotUrlChanged(const QUrl &url);

private Q_SLOTS:
    void genNewTabRequest(const QModelIndex &index);

private:
    class Private;
    Private * const d;
    friend class Private;
};

}

#endif // VIEWCONTAINER_H
