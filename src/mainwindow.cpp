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

#include <QFileSystemWatcher>
#include <QProcess>
#include <QProgressBar>
#include <QMimeData>
#include <QToolTip>
#include <QClipboard>
#include <QMenuBar>
#include <QMessageBox>
#include <QList>
#include <KToolBar>
#include <QToolButton>
#include <QTimer>
#include <QDockWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QWindow>
#include <qmath.h>

#include <KF5/KWidgetsAddons/KCapacityBar>
#include <KF5/KIOCore/KIO/CopyJob>
#include <KF5/KIOCore/KDirNotify>
#include <KF5/KIOWidgets/kio/pastejob.h>
#include <KF5/KIOWidgets/kio/paste.h>
#include <KFilePlacesModel>
#include <KFileWidget>
#include <KDiskFreeSpaceInfo>
#include <KPropertiesDialog>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KActionCollection>
#include <KNewFileMenu>
#include <KStandardAction>
#include <KUrlNavigator>
#include <KParts/kde_terminal_interface.h>
#include <KService/KService>
#include <KParts/KParts/ReadOnlyPart>
#include <KParts/KParts/ReadWritePart>
#include <KParts/KParts/Part>
#include <KParts/Plugin>
#include <KMimeTypeTrader>
#include <QMimeDatabase>

#include "dialogs/configdialog.h"
#include "views/fileplacesview.h"
#include "viewcontainer.h"
#include "fsmodel.h"
#include "searchbox.h"
#include "tabbar.h"
#include "mainwindow.h"
#include "application.h"
#include "tabbar.h"
#include "iconprovider.h"
#include "actions.h"
#include "widgets.h"

using namespace KDFM;

class DockWidget : public QDockWidget
{
    Q_OBJECT
public:
    DockWidget(QWidget *parent, const QString &title, const Qt::WindowFlags &flags, const Qt::DockWidgetAreas areas)
        : QDockWidget(title, parent, flags)
        , m_isLocked(false)
    {
        setAllowedAreas(areas);
        setFeatures(DockWidgetClosable | DockWidgetFloatable);
        setContentsMargins(0, 0, 0, 0);
        show();
    }
    bool isLocked() const { return m_isLocked; }

public slots:
    void toggleVisibility() { setVisible(!isVisible()); }
    void toggleLock() { setLocked(!m_isLocked); }
    void setLocked(const bool locked = false)
    {
        if (isFloating())
            return;
        if (locked)
        {
            QLabel *w = new QLabel(this);
            setTitleBarWidget(w);
            w->setContentsMargins(0, -w->fontMetrics().height(), 0, -w->fontMetrics().height());
            m_isLocked = true;
        }
        else
        {
            if (QWidget *w = titleBarWidget())
                delete w;
            setTitleBarWidget(0);
            m_isLocked = false;
        }
    }
private:
    bool m_isLocked;
};


Container
*Container::createContainer(const QUrl &url, QWidget *parent)
{
    static QMimeDatabase s_mimes;
    QMimeType mimeType = s_mimes.mimeTypeForFile(url.path());
    if (!mimeType.name().contains("text"))
        return 0;
    return new Container(url, mimeType, parent);
}
QString Container::title() const { return m_url.fileName(); }
QMimeType Container::mimeType() const { return m_mimeType; }
QUrl Container::url() const { return m_url; }

Container::Container(const QUrl &url, const QMimeType &mt, QWidget *parent)
    : QWidget(parent)
    //        , Configurable()
    , m_url(url)
    , m_mimeType(mt)
{
    KService::Ptr service = KService::serviceByDesktopName("katepart");
    KParts::ReadWritePart* p = service->createInstance<KParts::ReadWritePart>(0);
    if (!p)
        return;
    p->openUrl(url);
    QHBoxLayout *l = new QHBoxLayout(this);
    l->setContentsMargins(0,0,0,0);
    if (p->widget())
        l->addWidget(p->widget());
    setLayout(l);
}

class MainWindow::Private
{
public:
    Private(MainWindow *win) : q(win) {}
    MainWindow * const q;
    KDFM::StatusBar *statusBar;
    DockWidget *placesDock;
    QString statusMessage, slctnMessage;
    QItemSelection *currentSelection;
    QSlider *iconSizeSlider;
    FilePlacesView *placesView;
    TabManager *tabManager;
    TabBar *tabBar;
    QMainWindow *tabWin; //cheap sane ordering w/ tabbar horizontally over the whole window and docks *UNDER* tabbar
    QMenu *mainMenu;
    ActionContainer *actionContainer;
    KCapacityBar *capacityBar;
    QLabel *statusLabel[2];
    TerminalInterface *terminal;
    DockWidget *terminalDock;
    SearchBox *searchBox;
    DBusAdaptor *adaptor;
    struct Config {
        QString startPath;
        struct Views
        {
            struct IconView
            {
                int textLines;
            } icon;
        } views;
    } conf;
    void updateActions()
    {
        ViewContainer *c = q->activeContainer();
        if (!c)
            return;
        actionContainer->action(c->currentViewAction())->setChecked(true);
        Tab *t = static_cast<Tab *>(c->parentWidget());
        actionContainer->action(ActionContainer::SplitView)->setChecked(t->isSplitted());
        const int sort = c->model()->sortColumn();
        switch (sort)
        {
        case 0: actionContainer->action(ActionContainer::SortName)->setChecked(true); break;
        case 1: actionContainer->action(ActionContainer::SortSize)->setChecked(true); break;
        case 2: actionContainer->action(ActionContainer::SortDate)->setChecked(true); break;
        case 3: actionContainer->action(ActionContainer::SortPerm)->setChecked(true); break;
        default: break;
        }
        actionContainer->action(ActionContainer::SortDescending)->setChecked(c->model()->sortOrder() == Qt::DescendingOrder);
        actionContainer->action(ActionContainer::GoBack)->setEnabled(c->canGoBack());
        actionContainer->action(ActionContainer::GoForward)->setEnabled(c->canGoForward());
    }
    void showConfigDialog()
    {
        ConfigDialog dialog;
        if (dialog.exec() == QDialog::Accepted)
        {
            for (int i = 0; i < Configurable::configurables().count(); ++i)
                Configurable::configurables().at(i)->reconfigure();
        }
    }
};

MainWindow::MainWindow(const QStringList &arguments)
    : KParts::MainWindow()
    , d(new MainWindow::Private(this))
{
    setObjectName("KDFM::MainWindow#");
    d->actionContainer = new ActionContainer(this);
    d->tabWin = new QMainWindow(this, Qt::Widget);
    d->statusBar = new KDFM::StatusBar(this);
    d->placesDock = new DockWidget(d->tabWin, tr("Places"), Qt::SubWindow, Qt::LeftDockWidgetArea);
    d->placesView = new FilePlacesView();
    d->iconSizeSlider = new QSlider(d->statusBar);
    d->tabBar = new TabBar(this);
    d->tabManager = new TabManager(d->tabWin);
    d->tabManager->setTabBar(d->tabBar);
    d->tabWin->setWindowFlags(Qt::Widget);
    d->placesDock->setWidget(d->placesView);
    d->placesDock->setObjectName(tr("Places"));
    d->placesDock->setMinimumWidth(128);
    d->capacityBar = new KCapacityBar();
    d->capacityBar->setMinimumWidth(128);
    d->statusLabel[0] = new QLabel();
    d->statusLabel[1] = new QLabel();
    d->terminal = 0;
    d->terminalDock = 0;
    d->searchBox = 0;
    d->adaptor = new DBusAdaptor(this);

    QDBusConnection::sessionBus().registerService("org.kde.kdfm");
    QDBusConnection::sessionBus().registerObject("/KdfmAdaptor", this);

//    setAttribute(Qt::WA_TranslucentBackground, true); //neded for konsolepart these days.. ye, its argb, so cant embed argb win inside rgb win
//    setAttribute(Qt::WA_NoSystemBackground, false);

    setWindowIcon(QIcon::fromTheme("folder", QIcon(":/trolltech/styles/commonstyle/images/diropen-128.png")));

    connect(d->tabManager, &TabManager::newTabRequest, this, &MainWindow::newTab);
    connect(d->tabManager, &TabManager::currentWidgetChanged, this, &MainWindow::slotTabChanged);
    connect(d->tabManager, &TabManager::tabCloseRequested, this, &MainWindow::tabCloseRequest);

    connect(d->placesView, &FilePlacesView::urlChanged, this, [this](const QUrl &url) {activeContainer()->urlNav()->setLocationUrl(url);});
    connect(d->placesView, &FilePlacesView::newTabRequest, this, &MainWindow::addTab);

    connect(d->actionContainer->newFileMenu()->popupMenu(), &QMenu::aboutToShow, this, [this]()
    {
        QUrl url = activeContainer()->currentView()->rootIndex().data(FS::DirModel::FileItemRole).value<KFileItem>().url();
        if (!url.isValid())
            url = activeContainer()->rootUrl();
        d->actionContainer->newFileMenu()->setPopupFiles(url);
        d->actionContainer->newFileMenu()->checkUpToDate();
    });

    setStandardToolBarMenuEnabled(true);
    setupTerminal();
    setupStatusBar();

    d->tabBar->setDocumentMode(true);
    d->tabWin->setCentralWidget(d->tabManager);
    d->tabWin->addDockWidget(Qt::LeftDockWidgetArea, d->placesDock);

    for (int i = 1; i<arguments.count(); ++i)
    {
        const QString &argument = arguments.at(i);
        if (QFileInfo(argument).isDir())
            addTab(QUrl::fromLocalFile(argument));
    }
    if (d->tabBar->count() == 0)
    {
        KSharedConfigPtr config = KSharedConfig::openConfig("kdfm.conf");
        KConfigGroup general = config->group("General");
        QString startPath = general.readEntry("Location");
        if (startPath.isEmpty())
            startPath = QDir::homePath();
        addTab(QUrl::fromLocalFile(startPath));
    }

    QFrame *center = new QFrame(this);
    center->setFrameStyle(0);

    QVBoxLayout *vBox(new QVBoxLayout());
    vBox->setMargin(0);
    vBox->setSpacing(0);
    vBox->addWidget(d->tabBar);
    vBox->addWidget(d->tabWin);
    center->setLayout(vBox);

    setCentralWidget(center);
    setUnifiedTitleAndToolBarOnMac(true);
    setStatusBar(d->statusBar);

    d->actionContainer->action(ActionContainer::ShowMenuBar)->setChecked(true);
//    Actions::action(Actions::ShowPathBar)->setChecked(Store::settings()->value("pathVisible", true).toBool());
    activeContainer()->setFocus();
    updateIcons();
    setupGUI();
    connectActions();
}

MainWindow::~MainWindow()
{
    delete d;
}

void
MainWindow::setupTerminal()
{
    KService::Ptr service = KService::serviceByDesktopName("konsolepart");
    if (!service)
        return;
    // create one instance of konsolepart
    KParts::ReadOnlyPart* p = service->createInstance<KParts::ReadOnlyPart>(this, this, QVariantList());
    if (!p)
        return;
    // cast the konsolepart to the TerminalInterface..
    TerminalInterface* t = qobject_cast<TerminalInterface*>(p);
    if (!t)
        return;

    d->terminalDock = new DockWidget(this, "terminal", Qt::SubWindow, Qt::BottomDockWidgetArea);
    d->terminalDock->setObjectName("terminalDock");
    d->terminalDock->setWidget(p->widget());
    d->terminal = t;
    addDockWidget(Qt::BottomDockWidgetArea, d->terminalDock, Qt::Horizontal);
}

void
MainWindow::setSliderPos(const QSize &size)
{
    d->iconSizeSlider->blockSignals(true);
    d->iconSizeSlider->setValue(size.width()/16);
    d->iconSizeSlider->blockSignals(false);
}

void
MainWindow::setViewIconSize(int size)
{
    activeContainer()->setIconSize(size*16);
    d->iconSizeSlider->setToolTip(QString("Size: %1 px").arg(QString::number(size*16)));
    QToolTip *tip;
    QPoint pt;
    pt.setX(mapToGlobal(d->iconSizeSlider->pos()).x());
    pt.setY(mapToGlobal(d->statusBar->pos()).y());
    if (d->statusBar->isVisible())
        tip->showText(pt, d->iconSizeSlider->toolTip());
}

void
MainWindow::mainSelectionChanged()
{
    updateStatusBar(activeContainer());
}

void
MainWindow::setClipBoard()
{
    QMimeData *mimeData = activeContainer()->model()->mimeData(activeContainer()->selectedItems());
    QApplication::clipboard()->setMimeData(mimeData);
    KIO::setClipboardDataCut(mimeData, sender() == d->actionContainer->action(ActionContainer::Cut));
}

void
MainWindow::pasteSelection()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    QUrl url = activeContainer()->currentView()->rootIndex().data(FS::DirModel::FileItemRole).value<KFileItem>().url();
    if (!url.isValid())
        url = activeContainer()->rootUrl();
    KIO::paste(mimeData, url);
}

QString prettySize(qulonglong bytes, QString &type)
{
    if (!bytes)
        return QString();
    qreal size = (qreal)bytes;
    static QString types[] = { "Byte", "KB", "MB", "GB", "TB" };
    int i = 0;
    type = types[i];
    while (true)
    {
        qreal trySize = size/1024.0;
        if (trySize < 1.0 || i > 3)
            break;
        ++i;
        type = types[i];
        size = trySize;
    }
    return QString::number(size, 'f', 2);
}

void
MainWindow::slotTabChanged(QWidget *w, Tab *t)
{
    if (Container *cont = qobject_cast<Container *>(w))
    {
        updateCapacityBar(cont->url());
        updateStatusBar();
        setWindowTitle(cont->title());
        d->updateActions();
    }
    ViewContainer *c = qobject_cast<ViewContainer *>(w);
    if (!c)
        return;
    setWindowTitle(t->title());
    c->setPathEditable(d->actionContainer->action(ActionContainer::EditPath)->isChecked());
    d->iconSizeSlider->setValue(activeContainer()->iconSize().width()/16);
    d->iconSizeSlider->setToolTip(QString("Size: %1 px").arg(QString::number(c->iconSize().width())));
    d->placesView->setUrl(c->rootUrl());
    c->setFocus();
    if (d->searchBox)
        d->searchBox->setText(c->model()->nameFilter());
    d->updateActions();
    mainSelectionChanged();
    updateCapacityBar(c->rootUrl());
    updateStatusBar(c);
    if (d->terminal && c->rootUrl().isLocalFile())
        d->terminal->sendInput(QString("clear && cd %1\n").arg(sanitizeTerminalUrl(c->rootUrl().path())));
}

void
MainWindow::slotUrlItemsChanged()
{
    if (sender() == activeContainer())
    {
        updateStatusBar(activeContainer());
        updateCapacityBar(activeContainer()->rootUrl());
    }
}

void
MainWindow::updateStatusBar(ViewContainer *c)
{
    d->iconSizeSlider->setVisible(c);
    if (!c)
    {
        if (Container *cont = qobject_cast<Container *>(activeWidget()))
        {
            d->statusLabel[0]->clear();
            d->statusLabel[1]->clear();
            d->statusMessage = cont->url().toString();
            d->statusBar->setMessage(d->statusMessage);
            updateCapacityBar(cont->url());
        }
        return;
    }
    int dirCount, fileCount;
    qulonglong fileSize;
    c->model()->count(dirCount, fileCount, fileSize);
    QString type;
    QString size = prettySize(fileSize, type);
    QString text;
    if (dirCount)
        text.append(QString("%1 dirs").arg(QString::number(dirCount)));
    if (fileCount)
    {
        if (dirCount)
            text.append(", ");
        text.append(QString("%1 files(%2 %3)").arg(QString::number(fileCount)).arg(size).arg(type));
    }
    d->statusLabel[1]->setText(text);

    d->statusMessage = c->rootUrl().toString();
    if (c->rootUrl().scheme() == "file")
        d->statusMessage = c->rootUrl().path();

    const QModelIndexList &selected = activeContainer()->selectedItems();
    if (selected.count() == 1)
        d->slctnMessage = QString("\'%1\' Selected").arg(selected.first().data().toString());
    else if (selected.count() > 1)
        d->slctnMessage = QString("%1 Items Selected").arg(QString::number(selected.count()));

    if (selected.isEmpty())
        d->statusLabel[0]->clear();
    else
        d->statusLabel[0]->setText(d->slctnMessage);
    d->statusBar->setMessage(d->statusMessage);
}

void
MainWindow::updateCapacityBar(const QUrl &url)
{
    KDiskFreeSpaceInfo info = KDiskFreeSpaceInfo::freeSpaceInfo(url.path());
    if (!info.isValid())
    {
        d->capacityBar->setValue(0);
        d->capacityBar->setText("");
        return;
    }
    int percentage = 0;
    if (info.used())
        percentage = (qreal)info.used()/(qreal)info.size()*100.0;
    d->capacityBar->setValue(percentage);
    QString type;
    QString used = prettySize(info.available(), type);
    d->capacityBar->setText(QString("%1 %2 free").arg(used).arg(type));
}

void
MainWindow::slotUrlChanged(const QUrl &url)
{
    if (!sender())
        return;
    ViewContainer *c = static_cast<ViewContainer *>(sender()); //only viewcontainers send urlchanged... hence static_cast
    Tab *t = static_cast<Tab *>(c->parentWidget());
    if (t->isSplitted())
    {
        ViewContainer *c1 = static_cast<ViewContainer *>(t->widget1());
        ViewContainer *c2 = static_cast<ViewContainer *>(t->widget2());
        t->setTitle(QString("%1 | %2").arg(c1->title(), c2->title()));
    }
    else
        t->setTitle(c->title());
    t->setIcon(QIcon::fromTheme(c->rootItem().iconName()));
    if (c == activeContainer())
    {
        d->placesView->setUrl(url);
        setWindowTitle(t->title());
        if (d->searchBox)
            d->searchBox->setText(QString());
        updateStatusBar(c);
        d->actionContainer->action(ActionContainer::GoBack)->setEnabled(c->canGoBack());
        d->actionContainer->action(ActionContainer::GoForward)->setEnabled(c->canGoForward());
    }
    if (d->terminal && url.isLocalFile())
        d->terminal->sendInput(QString("clear && cd %1\n").arg(sanitizeTerminalUrl(url.path())));
    updateCapacityBar(url);
}

void
MainWindow::slotUrlLoaded(const QUrl &url)
{
    if (sender() == activeContainer())
    {
        updateStatusBar(activeContainer());
    }
}

void MainWindow::setView()
{
    ViewContainer::View view = ViewContainer::Icon;
    if (sender() == d->actionContainer->action(ActionContainer::Views_Detail))
        view = ViewContainer::Details;
    else if (sender() == d->actionContainer->action(ActionContainer::Views_Column))
        view = ViewContainer::Column;
    else if (sender() == d->actionContainer->action(ActionContainer::Views_Flow))
        view = ViewContainer::Flow;
    activeContainer()->setView(view);
}

void
MainWindow::slotToggleVisible()
{
    QAction *a = qobject_cast<QAction *>(sender());
    if (!a)
        return;
    const bool visible = a->isChecked();
    if (a == d->actionContainer->action(ActionContainer::ShowPathBar))
    {
        foreach (KUrlNavigator *nav, findChildren<KUrlNavigator *>())
            nav->setVisible(visible);
//        Store::settings()->setValue("pathVisible", visible);
    }
    else
    {
        menuBar()->setVisible(visible);
    }
}

ViewContainer
*MainWindow::createViewContainer(const QUrl &url, QWidget *containerParent)
{
    ViewContainer *container = new ViewContainer(containerParent, static_cast<KFilePlacesModel *>(d->placesView->model()), url);
    container->setIconSize(3/*Store::config.views.iconView.iconSize*/*16);
    connect(container, &ViewContainer::selectionChanged, this, &MainWindow::mainSelectionChanged);
    connect(container, &ViewContainer::iconSizeChanged, this, &MainWindow::setSliderPos);
    connect(container, &ViewContainer::newTabRequest, this, &MainWindow::addTab);
    connect(container, &ViewContainer::urlChanged, this, &MainWindow::slotUrlChanged);
    connect(container, &ViewContainer::urlLoaded, this, &MainWindow::slotUrlLoaded);
    connect(container, &ViewContainer::urlItemsChanged, this, &MainWindow::slotUrlItemsChanged);

    connect(container->model(), &FS::ProxyModel::layoutChanged, this, [this]()
    {
       d->updateActions();
    });
    return container;
}

void
MainWindow::splitCurrentTab(const bool split)
{
    Tab *t = d->tabManager->currentTab();
    if (!activeContainer())
        return;
    if (split)
    {
        t->split(createViewContainer(activeContainer()->rootUrl()));
        ViewContainer *c1 = static_cast<ViewContainer *>(t->widget1());
        ViewContainer *c2 = static_cast<ViewContainer *>(t->widget2());
        t->setTitle(QString("%1 | %2").arg(c1->title(), c2->title()));
        setWindowTitle(t->title());
    }
    else
    {
        t->unsplit();
        t->setTitle(static_cast<ViewContainer *>(t->lastFocused())->title());
        setWindowTitle(t->title());
    }
}

void
MainWindow::tabCloseRequest(int tab)
{
    d->tabManager->deleteTab(tab);
    if (activeContainer())
    {
        activeContainer()->setFocus();
        updateCapacityBar(activeContainer()->rootUrl());
        updateStatusBar(activeContainer());
        if (d->terminal)
            d->terminal->sendInput(QString("clear && cd %1\n").arg(sanitizeTerminalUrl(activeContainer()->rootUrl().path())));
    }
    d->updateActions();
}

void
MainWindow::addTab(const QUrl &url)
{
    KFileItem file(url);
    if (file.isDir())
    {
        static QIcon dirIcon;
        if (dirIcon.isNull())
        {
            QStyleOption opt;
            opt.initFrom(this);
            opt.rect = QRect(0, 0, 16, 16);
            dirIcon = style()->standardIcon(QStyle::SP_DirIcon, &opt, this);
        }
        d->tabManager->addTab(createViewContainer(url), dirIcon, url.fileName(), -1/*d->tabManager->indexOf(activeContainer())+1*/);
        return;
    }
    if (Container *cont = Container::createContainer(url))
        d->tabManager->addTab(cont, QIcon::fromTheme(cont->mimeType().genericIconName()), cont->title());
}

void
MainWindow::openTab()
{
    const QModelIndexList &indexes = activeContainer()->selectedItems();
    for (int i = 0; i < indexes.size(); ++i)
    {
        const QModelIndex &index = indexes.at(i);
        const QUrl &url = index.data(FS::DirModel::FileItemRole).value<KFileItem>().url();
        addTab(url);
    }
}

void
MainWindow::newTab()
{ addTab(activeContainer()->rootUrl()); }

QWidget
*MainWindow::activeWidget() const
{
    Tab *t = d->tabManager->tab(d->tabManager->currentIndex());
    return t->lastFocused();
}

ViewContainer
*MainWindow::activeContainer() const
{
    return qobject_cast<ViewContainer *>(activeWidget());
}

FilePlacesView
*MainWindow::placesView() const
{ return d->placesView; }

QMenu
*MainWindow::mainMenu() const
{ return d->mainMenu; }

QSlider
*MainWindow::iconSizeSlider() const
{ return d->iconSizeSlider; }

TabBar
*MainWindow::tabBar() const
{ return d->tabBar; }

ActionContainer
*MainWindow::actionContainer() const
{ return d->actionContainer; }

void
MainWindow::showConfigDialog()
{ d->showConfigDialog(); }

void
MainWindow::setSorting()
{
    int i = 0;
    if (d->actionContainer->action(ActionContainer::SortName)->isChecked())
        i = 0;
    else if (d->actionContainer->action(ActionContainer::SortSize)->isChecked())
        i = 1;
    else if (d->actionContainer->action(ActionContainer::SortDate)->isChecked())
        i = 2;
    else if (d->actionContainer->action(ActionContainer::SortPerm)->isChecked())
        i = 3;
    activeContainer()->sort(i, (Qt::SortOrder)d->actionContainer->action(ActionContainer::SortDescending)->isChecked());
}

void
MainWindow::filterCurrentTab(const QString &filter)
{
    if (actionContainer())
        activeContainer()->model()->slotFilterByName(filter);
}

void
MainWindow::updateIcons()
{
#define SETICON(_ICON_) setIcon(IconProvider::icon(IconProvider::_ICON_))
    d->actionContainer->action(ActionContainer::GoBack)->SETICON(GoBack);
    d->actionContainer->action(ActionContainer::GoForward)->SETICON(GoForward);
    d->actionContainer->action(ActionContainer::Views_Icon)->SETICON(IconView);
    d->actionContainer->action(ActionContainer::Views_Detail)->SETICON(DetailsView);
    d->actionContainer->action(ActionContainer::Views_Column)->SETICON(ColumnsView);
    d->actionContainer->action(ActionContainer::Views_Flow)->SETICON(FlowView);
    d->actionContainer->action(ActionContainer::Configure)->SETICON(Configure);
    d->actionContainer->action(ActionContainer::GoHome)->SETICON(GoHome);
    d->actionContainer->action(ActionContainer::ShowHidden)->SETICON(Hidden);
    d->actionContainer->action(ActionContainer::Sort)->SETICON(Sort);
#undef SETICON
}

void
MainWindow::connectActions()
{
    if (QWidgetAction *wa = qobject_cast<QWidgetAction *>(d->actionContainer->action(ActionContainer::Filter)))
    {
        if (SearchBox *box = qobject_cast<SearchBox *>(wa->defaultWidget()))
        {
            d->searchBox = box;
            connect(d->searchBox, &SearchBox::textChanged, this, &MainWindow::filterCurrentTab);
        }
    }
    QMenu *helpMenu = menuBar()->findChild<QMenu*>("help");
    if (!helpMenu) {
        helpMenu = menuBar()->addMenu(tr("&Help"));
        helpMenu->setObjectName("help");
    }

    QAction *aboutAction = new QAction(tr("About the Document Surfer"), this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutInfo);
    helpMenu->addAction(aboutAction);
    connect(d->actionContainer->action(ActionContainer::DeleteSelection), &QAction::triggered, this, [this](){activeContainer()->deleteCurrentSelection();});
    connect(d->actionContainer->action(ActionContainer::GoHome), &QAction::triggered, this, [this](){activeContainer()->goHome();});
    connect(d->actionContainer->action(ActionContainer::GoBack), &QAction::triggered, this, [this](){activeContainer()->goBack();});
    connect(d->actionContainer->action(ActionContainer::GoUp), &QAction::triggered, this, [this](){activeContainer()->goUp();});
    connect(d->actionContainer->action(ActionContainer::GoForward), &QAction::triggered, this, [this](){activeContainer()->goForward();});
    connect(d->actionContainer->action(ActionContainer::Exit), &QAction::triggered, this, &MainWindow::close);
    connect(d->actionContainer->action(ActionContainer::Views_Icon), &QAction::triggered, this, &MainWindow::setView);
    connect(d->actionContainer->action(ActionContainer::Views_Detail), &QAction::triggered, this, &MainWindow::setView);
    connect(d->actionContainer->action(ActionContainer::Views_Column), &QAction::triggered, this, &MainWindow::setView);
    connect(d->actionContainer->action(ActionContainer::Views_Flow), &QAction::triggered, this, &MainWindow::setView);
    connect(d->actionContainer->action(ActionContainer::SplitView), &QAction::triggered, this, &MainWindow::splitCurrentTab);
    connect(d->actionContainer->action(ActionContainer::ShowHidden), &QAction::triggered, this, [this](){activeContainer()->model()->dirLister()->setShowingDotFiles(d->actionContainer->action(ActionContainer::ShowHidden)->isChecked());});
//    connect(Actions::action(Actions::MkDir), &QAction::triggered, this, [this](){activeContainer()->createDirectory();});
    connect(d->actionContainer->action(ActionContainer::Copy), &QAction::triggered, this, &MainWindow::setClipBoard);
    connect(d->actionContainer->action(ActionContainer::Cut), &QAction::triggered, this, &MainWindow::setClipBoard);
    connect(d->actionContainer->action(ActionContainer::Paste), &QAction::triggered, this, &MainWindow::pasteSelection);
    connect(d->actionContainer->action(ActionContainer::Rename), &QAction::triggered, this, [this](){activeContainer()->rename();});
    connect(d->actionContainer->action(ActionContainer::Refresh), &QAction::triggered, this, [this](){activeContainer()->refresh();});
//    connect(Actions::action(Actions::ShowToolBar), &QAction::toggled, this, &MainWindow::slotToggleVisible);
    connect(d->actionContainer->action(ActionContainer::ShowMenuBar), &QAction::toggled, this, &MainWindow::slotToggleVisible);
//    connect(Actions::action(Actions::ShowStatusBar), &QAction::toggled, this, &MainWindow::slotToggleVisible);
    connect(d->actionContainer->action(ActionContainer::ShowPathBar), &QAction::toggled, this, &MainWindow::slotToggleVisible);
    connect(d->actionContainer->action(ActionContainer::AddTab), &QAction::triggered, this, &MainWindow::newTab);
    connect(d->actionContainer->action(ActionContainer::OpenInTab), &QAction::triggered, this, &MainWindow::openTab);
    connect(d->actionContainer->action(ActionContainer::Configure), &QAction::triggered, this, &MainWindow::showConfigDialog);
    connect(d->actionContainer->action(ActionContainer::Properties), &QAction::triggered, this, [this](){KPropertiesDialog::showDialog(activeContainer()->selectedUrls(), window());});
    connect(d->actionContainer->action(ActionContainer::RestoreFromTrashAction), &QAction::triggered, this, [this](){activeContainer()->restoreFromTrash();});
    connect(d->actionContainer->action(ActionContainer::MoveToTrashAction), &QAction::triggered, this, [this]() {activeContainer()->moveToTrash();});
    connect(d->actionContainer->action(ActionContainer::EditPath), &QAction::triggered, this, [this](){activeContainer()->setPathEditable(d->actionContainer->action(ActionContainer::EditPath)->isChecked());});
    connect(d->actionContainer->action(ActionContainer::SortDescending), &QAction::triggered, this, &MainWindow::setSorting);
    foreach (QAction *a, d->actionContainer->sortActs()->actions())
        connect(a, &QAction::triggered, this, &MainWindow::setSorting);
}

void
MainWindow::setupStatusBar()
{
    d->statusBar->addRightWidget(d->statusLabel[1]);

    d->iconSizeSlider->setFixedWidth(80);
    d->iconSizeSlider->setOrientation(Qt::Horizontal);
    d->iconSizeSlider->setRange(1,16);
    d->iconSizeSlider->setSingleStep(1);
    d->iconSizeSlider->setPageStep(1);
    d->statusBar->addRightWidget(d->iconSizeSlider);
    connect(d->iconSizeSlider, &QSlider::valueChanged, this, &MainWindow::setViewIconSize);

    QToolButton *placesBtn = new QToolButton(d->statusBar);
    placesBtn->setIcon(QIcon::fromTheme("inode-directory"));
    placesBtn->setIconSize(QSize(16, 16));
    placesBtn->setCheckable(true);
    d->statusBar->addLeftWidget(placesBtn);
    connect(placesBtn, &QToolButton::clicked, d->placesDock, &DockWidget::toggleVisibility);
    connect(d->placesDock, &DockWidget::visibilityChanged, placesBtn, &QToolButton::setChecked);

    if (d->terminalDock)
    {
        QToolButton *terminalBtn = new QToolButton(d->statusBar);
        terminalBtn->setIcon(QIcon::fromTheme("terminal"));
        terminalBtn->setIconSize(QSize(16, 16));
        terminalBtn->setCheckable(true);
        d->statusBar->addLeftWidget(terminalBtn);
        connect(terminalBtn, &QToolButton::clicked, d->terminalDock, &DockWidget::toggleVisibility);
        connect(d->terminalDock, &DockWidget::visibilityChanged, terminalBtn, &QToolButton::setChecked);
//        connect(d->terminalDock, &DockWidget::visibilityChanged, this, [this](const bool visible)
//        {
//            if (visible && d->terminalDock->widget())
//                setupTerminal();

//        });
    }

    d->placesDock->setLocked(true);
    QToolButton *dockLock = new QToolButton(d->statusBar);
    dockLock->setIcon(QIcon::fromTheme("emblem-locked"));
    dockLock->setIconSize(QSize(16, 16));
    dockLock->setCheckable(true);
    dockLock->setChecked(d->placesDock->isLocked());
    d->statusBar->addLeftWidget(dockLock);
    connect(dockLock, &QToolButton::clicked, d->placesDock, &DockWidget::setLocked);
    if (d->terminalDock)
    {
        d->terminalDock->setLocked(true);
        connect(dockLock, &QToolButton::clicked, d->terminalDock, &DockWidget::setLocked);
    }
    connect(dockLock, &QToolButton::clicked, this, [this](const bool lock){static_cast<QToolButton*>(sender())->setIcon(QIcon::fromTheme(lock?"emblem-locked":"emblem-unlocked"));});
    d->statusBar->addLeftWidget(d->capacityBar);
    d->statusBar->addLeftWidget(d->statusLabel[0]);
}

QString
MainWindow::sanitizeTerminalUrl(QString url)
{
    QString ret = url;
    ret.replace(" ", "\\ ");
    return ret;
}

void MainWindow::showAboutInfo()
{
    DocSurfAboutInfo *about = new DocSurfAboutInfo(this);
    about->exec();  // or .show() if it's not modal
}

#include "mainwindow.moc"
