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

#include <QStyle>
#include <QMouseEvent>
#include <QDebug>
#include <QStyleOption>
#include <QPainter>
#include <QPoint>
#include <QApplication>

#include <KF5/KConfigCore/KSharedConfig>
#include <KF5/KConfigCore/KConfigGroup>

#include "tabbar.h"
#include "viewcontainer.h"

using namespace DocSurf;

TabBar::TabBar(QWidget *parent)
    : QTabBar(parent)
{
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setDocumentMode(true);
    setTabsClosable(true);
    setMovable(true);
    setDrawBase(true);
//    setExpanding(!Store::config.behaviour.gayWindow);
    setElideMode(Qt::ElideRight);
    setAcceptDrops(true);
    setTabsClosable(true);
}

void
TabBar::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::MiddleButton)
        emit tabCloseRequested(tabAt(e->pos()));
    else
        QTabBar::mousePressEvent(e);
}

void
TabBar::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton
            && tabAt(e->pos()) == -1)
        emit newTabRequest();
    QTabBar::mouseDoubleClickEvent(e);
}

//-------------------------------------------------------------------------------------------

class Handle::Private
{
public:
    Private(Handle *handle) : q(handle) {}
    Handle * const q;
    bool hasPress;
    int x;
};

Handle::Handle(QWidget *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    d->hasPress = false;
    setAttribute(Qt::WA_Hover);
    QStyleOption opt;
    opt.initFrom(this);
    setFixedWidth(qMax(8, style()->pixelMetric(QStyle::PM_SplitterWidth, &opt, this)));
    setCursor(Qt::SplitHCursor);
}

void
Handle::moveEvent(QMoveEvent *e)
{
    QWidget::moveEvent(e);
    if (d->hasPress)
    {
        QRect geo1 = parentWidget()->rect();
        geo1.setRight(geometry().x());
        QRect geo2 = parentWidget()->rect();
        geo2.setLeft(geometry().right());
        emit resizeColumnRequest(geo1, geo2);
    }
}

void
Handle::paintEvent(QPaintEvent *e)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    if (underMouse())
        opt.state |= QStyle::State_MouseOver;
    style()->drawControl(QStyle::CE_Splitter, &opt, &p, this);
}

void
Handle::mousePressEvent(QMouseEvent *e)
{
    e->accept();
    setMouseTracking(true);
    d->hasPress = true;
    d->x = mapToGlobal(QPoint()).x();
}

void
Handle::mouseReleaseEvent(QMouseEvent *e)
{
    e->accept();
    setMouseTracking(false);
    d->hasPress = false;
}

void
Handle::mouseMoveEvent(QMouseEvent *e)
{
    e->accept();
    if (d->hasPress)
    {
        move(geometry().x() + (e->globalX() - d->x), 0);
        if (geometry().x() != d->x)
            d->x = e->globalX();
    }
}


//-------------------------------------------------------------------------------------------

class Tab::Private
{
public:
    Private(Tab *t) : q(t) {}
    Tab * const q;
    Handle *handle;
    QWidget *widget_1;
    QWidget *widget_2;
    QWidget *lastFocused;
    QString title;
    TabManager *mgr;
    bool splitted;
};

Tab::Tab(QWidget *w, TabManager *parent)
    : QWidget(parent)
    , d(new Private(this))
{
    w->setParent(this);
    w->setFocus();
    d->widget_1 = w;
    d->widget_2 = 0;
    d->lastFocused = w; //we only have one widget... ergo, thats the last focused
    d->handle = new Handle(this);
    d->handle->hide();
    d->mgr = parent;
    d->splitted = false;
    connect(d->handle, &Handle::resizeColumnRequest, this, [this](const QRect &geo1, const QRect &geo2)
    {
        if (d->widget_1)
            d->widget_1->setGeometry(geo1);
        if (d->widget_2)
            d->widget_2->setGeometry(geo2);
    });
}

Tab::~Tab()
{
    delete d;
}

void
Tab::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    if (d->handle->isVisible())
    {
        d->handle->resize(d->handle->width(), height());
        qreal perc = (qreal)d->handle->geometry().x() / (qreal)e->oldSize().width();
        d->handle->move(width()*perc, 0);
    }

    if (!d->widget_2)
        d->widget_1->resize(width(), height());
    else
    {
        QRect geo1 = rect();
        geo1.setRight(d->handle->geometry().x());
        d->widget_1->setGeometry(geo1);
        QRect geo2 = rect();
        geo2.setLeft(d->handle->geometry().right());
        d->widget_2->setGeometry(geo2);
    }
}

void
Tab::focusChanged(QWidget *from, QWidget *to)
{
    if (!d->splitted)
        return;
    QPalette pal = qApp->palette();
    QColor text = pal.color(QPalette::Text);
    QColor winText = pal.color(QPalette::WindowText);
    text.setAlpha(127);
    winText.setAlpha(127);
    pal.setColor(QPalette::Text, text);
    pal.setColor(QPalette::WindowText, winText);
    if (d->widget_1->isAncestorOf(to) || d->widget_1 == to)
    {
        if (d->lastFocused != d->widget_1)
        {
            d->lastFocused = d->widget_1;
            d->widget_2->setPalette(pal);
            d->widget_1->setPalette(qApp->palette());
            emit d->mgr->currentWidgetChanged(d->widget_1, this);
        }
    }
    else if (d->widget_2->isAncestorOf(to) || d->widget_2 == to)
    {
        if (d->lastFocused != d->widget_2)
        {
            d->lastFocused = d->widget_2;
            d->widget_1->setPalette(pal);
            d->widget_2->setPalette(qApp->palette());
            emit d->mgr->currentWidgetChanged(d->widget_2, this);
        }
    }
}

void
Tab::split(QWidget *w)
{
    d->handle->show();
    d->handle->move(width()/2-d->handle->width()/2, 0);
    d->handle->resize(d->handle->width(), height());
    d->widget_2 = w;
    d->splitted = true;
    w->setParent(this);
    w->show();
    connect(qApp, &QApplication::focusChanged, this, &Tab::focusChanged);
    w->setFocus();
    QRect geo1 = rect();
    geo1.setRight(d->handle->geometry().x());
    d->widget_1->setGeometry(geo1);
    QRect geo2 = rect();
    geo2.setLeft(d->handle->geometry().right());
    d->widget_2->setGeometry(geo2);
}

void
Tab::unsplit()
{
    if (d->lastFocused == d->widget_2)
    {
        std::swap(d->widget_1, d->widget_2);
        d->lastFocused = d->widget_1;
    }
    disconnect(qApp, &QApplication::focusChanged, this, &Tab::focusChanged);
    d->widget_2->deleteLater();
    d->widget_2->hide();
    d->widget_2 = 0;
    d->handle->hide();
    d->widget_1->setGeometry(rect());
    d->lastFocused = d->widget_1;
    d->splitted = false;
}

void
Tab::setTitle(const QString &title)
{ d->mgr->tabBar()->setTabText(index(), title); }

QString
Tab::title() const
{ return d->mgr->tabBar()->tabText(index()); }

void
Tab::setIcon(const QIcon &icon)
{ d->mgr->tabBar()->setTabIcon(index(), icon); }

QIcon
Tab::icon() const
{ return d->mgr->tabBar()->tabIcon(index()); }

QWidget
*Tab::widget1() const
{ return d->widget_1; }

QWidget
*Tab::widget2() const
{ return d->widget_2; }

QWidget
*Tab::lastFocused() const
{ return d->lastFocused; }

bool
Tab::isSplitted() const
{ return d->splitted; }

int
Tab::index() const
{ return d->mgr->indexOf(const_cast<Tab *>(this)); }

//-------------------------------------------------------------------------------------------

TabManager::TabManager(QWidget *parent)
    : QStackedWidget(parent)
    , Configurable()
    , m_tabBar(0)
    , m_hideWhenOne(true)
{
    QMetaObject::invokeMethod(this, &TabManager::reconfigure, Qt::QueuedConnection);
}

void
TabManager::deleteTab(const int tab)
{
    if (count() <= 1)
        return;
    if (Tab *t = takeTab(tab))
        t->deleteLater();
    setCurrentIndex(m_tabBar->currentIndex());
    window()->setWindowTitle(m_tabBar->tabText(m_tabBar->currentIndex()));
}

void
TabManager::reconfigure()
{
    //Config pointer
    KSharedConfigPtr config = KSharedConfig::openConfig("NSEDocSurf.conf");

    //General
    KConfigGroup general = config->group("General");
    m_hideWhenOne = general.readEntry("HideTabBar", false);
    m_tabBar->setVisible(m_tabBar->count()>1 || !m_hideWhenOne);
}

bool
TabManager::tabSplitted(const int index) const
{ return tab(index)->isSplitted(); }

void
TabManager::splitTab(const int index, const bool split, QWidget *w)
{
    Tab *t = tab(index);
    if (split)
        t->split(w);
    else
        t->unsplit();
}

Tab
*TabManager::createTab(QWidget *w)
{ return new Tab(w, this); }

Tab
*TabManager::takeTab(const int tab)
{
    if (Tab *t = static_cast<Tab *>(widget(tab)))
    {
        blockSignals(true);
        removeWidget(t);
        blockSignals(false);
        m_tabBar->blockSignals(true);
        m_tabBar->removeTab(tab);
        m_tabBar->blockSignals(false);
        m_tabBar->setVisible(m_tabBar->count()>1 || !m_hideWhenOne);
        return t;
    }
    return 0;
}

Tab
*TabManager::currentTab() const
{ return tab(currentIndex()); }

Tab
*TabManager::tab(const int index) const
{ return static_cast<Tab *>(widget(index)); }

void
TabManager::setTabBar(TabBar *tabBar)
{
    m_tabBar = tabBar;
    connect(m_tabBar, &TabBar::tabMoved, this, &TabManager::tabMoved);
    connect(m_tabBar, &TabBar::currentChanged, this, &TabManager::setCurrentIndex);
    connect(m_tabBar, &TabBar::currentChanged, this, &TabManager::tabChanged);
    connect(m_tabBar, &TabBar::newTabRequest, this, &TabManager::newTabRequest);
    connect(m_tabBar, &TabBar::tabCloseRequested, this, &TabManager::tabCloseRequested);
    connect(m_tabBar, &TabBar::currentChanged, this, [this](const int index) { emit currentWidgetChanged(tab(index)->lastFocused(), tab(index)); });
}

void
TabManager::tabMoved(int from, int to)
{
    blockSignals(true);
    QWidget *w = widget(from);
    removeWidget(w);
    insertWidget(to, w);
    setCurrentIndex(m_tabBar->currentIndex());
    blockSignals(false);
}

int
TabManager::insertTab(int index, Tab *t, const QIcon &icon, const QString &text)
{
    if (!t)
        return -1;
    m_tabBar->show();
    index = insertWidget(index, t);
    index = m_tabBar->insertTab(index, icon, text);
    return index;
}

int
TabManager::addTab(QWidget *w, const QIcon &icon, const QString &text, const int index)
{
    return insertTab(index, createTab(w), icon, text);
}
