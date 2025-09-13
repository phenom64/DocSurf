/**************************************************************************
*   Original app DFM Copyright (C) 2013 by Robert Metsaranta                               *
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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KParts/MainWindow>
#include "dialogs/aboutInfo.h"
#include <QWidget>
#include <QMimeType>
#include <QUrl>
#include <QDBusAbstractAdaptor>

class QSlider;
namespace DocSurf
{

class Container : public QWidget/*, public Configurable*/
{
    Q_OBJECT
public:
    static Container *createContainer(const QUrl &url, QWidget *parent = 0);
    QString title() const;
    QMimeType mimeType() const;
    QUrl url() const;

protected:
    explicit Container(const QUrl &url, const QMimeType &mt, QWidget *parent = 0);
private:
    QUrl m_url;
    QMimeType m_mimeType;
};

class TabBar;
class ViewContainer;
class PlacesView;
class FilePlacesView;
class ActionContainer;
class Tab;
class MainWindow : public KParts::MainWindow
{
    Q_OBJECT
public:
    MainWindow(const QStringList &arguments = QStringList());
    ~MainWindow();
    ViewContainer *activeContainer() const;
    QWidget *activeWidget() const;
    FilePlacesView *placesView() const;
    QMenu *mainMenu() const;
    QSlider *iconSizeSlider() const ;
    TabBar *tabBar() const;
    ActionContainer *actionContainer() const;
    ViewContainer *createViewContainer(const QUrl &url, QWidget *containerParent = 0);
    static QString sanitizeTerminalUrl(QString url);

public slots:
    void addTab(const QUrl &url);
    void showConfigDialog();

Q_SIGNALS:
    void activeWidgetChanged();

protected:
    void updateCapacityBar(const QUrl &url);
    void setupStatusBar();
    void updateStatusBar(ViewContainer *c = 0);
    void setupTerminal();

private slots:
    void slotUrlChanged(const QUrl &url);
    void slotUrlLoaded(const QUrl &url);
    void slotUrlItemsChanged();
    void slotTabChanged(QWidget *w, Tab *t);
    void connectActions();
    void updateIcons();
    void setView();
    void showAboutInfo();
    void slotToggleVisible();
    void setClipBoard();
    void pasteSelection();
    void mainSelectionChanged();
    void setViewIconSize(int);
    void setSliderPos(const QSize &size);
    void openTab();
    void newTab();
    void tabCloseRequest(int);
    void setSorting();
    void splitCurrentTab(const bool split);
    void filterCurrentTab(const QString &filter);

private:
    class Private;
    Private * const d;
    friend class Private;
};

class DBusAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.syndromatic.docsurf")

public:
    DBusAdaptor(MainWindow *parent = 0) : QDBusAbstractAdaptor(parent), m_win(parent){}

public slots:
    Q_NOREPLY void openUrl(const QString &url) { m_win->addTab(QUrl::fromUserInput(url)); }

//signals:

private:
    MainWindow *m_win;
};


}
#endif
