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


#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>
#include <QStackedWidget>
#include <QIcon>
#include "widgets.h"

class QHBoxLayout;
class QToolBar;
class QToolButton;
namespace DocSurf
{
class ViewContainer;
class TabBar;

class TabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit TabBar(QWidget *parent = 0);
    ~TabBar(){}

signals:
    void newTabRequest();

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
};

class Handle : public QWidget
{
    Q_OBJECT
public:
    explicit Handle(QWidget *parent);

protected:
    void moveEvent(QMoveEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *e);

signals:
    void resizeColumnRequest(const QRect &geo1, const QRect &geo2);

private:
    class Private;
    Private * const d;
    friend class Private;
};

class TabManager;
class Tab : public QWidget
{
    Q_OBJECT
public:
    ~Tab();
    int index() const;

    void split(QWidget *w);
    void unsplit();
    bool isSplitted() const;

    void setTitle(const QString &title);
    QString title() const;

    void setIcon(const QIcon &icon);
    QIcon icon() const;

    QWidget *widget1() const;
    QWidget *widget2() const;
    QWidget *lastFocused() const;

protected:
    Tab(QWidget *w, TabManager *parent);
    void resizeEvent(QResizeEvent *e);

protected slots:
    void focusChanged(QWidget *from, QWidget *to);

private:
    class Private;
    Private * const d;
    friend class Private;
    friend class TabManager;
};

//class ViewContainerTab : public Tab
//{
//    Q_OBJECT
//public:
//    ViewContainerTab(ViewContainer *c, TabManager *mgr);
//};

class TabManager : public QStackedWidget, Configurable
{
    Q_OBJECT
public:
    explicit TabManager(QWidget *parent = 0);
    ~TabManager(){}

    inline TabBar *tabBar() { return m_tabBar; }
    void setTabBar(TabBar *tabBar);
    int addTab(QWidget *w, const QIcon& icon = QIcon(), const QString &text = QString(), const int index = -1);
    int insertTab(int index, Tab *t, const QIcon& icon = QIcon(), const QString &text = QString());
    Tab *createTab(QWidget *w);
    Tab *tab(const int index) const;
    Tab *takeTab(const int index);
    Tab *currentTab() const;
    void deleteTab(const int index);
    bool tabSplitted(const int index) const;
    void splitTab(const int index, const bool split, QWidget *w = 0);

signals:
    void tabChanged(int index);
    //the currentWidget is the one that is focused and showed,
    //this is changed whenever the user switches tabs or clicks
    //inside the widget of a splitted tab
    void currentWidgetChanged(QWidget *w, Tab *t);
    void newTabRequest();
    void tabCloseRequested(int index);

public slots:
    void reconfigure();

private slots:
    void tabMoved(int from, int to);

private:
    TabBar *m_tabBar;
    bool m_hideWhenOne;
};


}

#endif // TABBAR_H
