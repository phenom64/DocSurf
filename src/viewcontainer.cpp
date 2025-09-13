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

#include <QProcess>
#include <QMap>
#include <QInputDialog>
#include <QCompleter>
#include <QMenu>
#include <QDesktopServices>
#include <QAbstractItemView>
#include <QStackedLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QStyledItemDelegate>
#include <QModelIndexList>
#include <QMenu>
#include <QDebug>

#include <KF5/KJobWidgets/KJobWidgets>
#include <KF5/KIOCore/KIO/DeleteJob>
#include <KF5/KIOCore/KFileItemListProperties>
#include <KF5/KIOCore/KFileItem>
#include <KF5/KConfigCore/KDesktopFile>
#include <KF5/KIOWidgets/KRun>
#include <KF5/KIOWidgets/KPropertiesDialog>
#include <KF5/KIOWidgets/KFileItemActions>
#include <KF5/KIOWidgets/KIO/JobUiDelegate>
#include <KF5/KIOFileWidgets/KFilePlacesModel>
#include <KF5/KIOFileWidgets/KDirOperator>
#include <KF5/KIOFileWidgets/KNewFileMenu>
#include <KF5/KIOFileWidgets/KUrlNavigator>
#include <KF5/KIOCore/KIO/EmptyTrashJob>
#include <KF5/KIOCore/KIO/RestoreJob>
#include <KF5/KIOCore/KIO/CopyJob>
#include <KF5/KIOCore/KIO/StatJob>

#include <KConfigCore/KConfigGroup>
#include <KSharedConfig>

#include "viewcontainer.h"
#include "views/iconview.h"
#include "views/detailsview.h"
#include "views/flowview.h"
#include "views/columnview.h"
#include "views/fileplacesview.h"
#include "flow.h"
#include "fsmodel.h"
#include "mainwindow.h"
#include "tabbar.h"
#include "searchbox.h"
#include "actions.h"

using namespace DocSurf;

QString
ViewContainer::prettySize(quint64 bytes)
{
    if (bytes & (0x3ffful<<50))
        return QString::number((bytes>>50) + ((bytes>>40) & (0x3fful)) / 1024.0, 'f', 2) + " PB";
    else if (bytes & (0x3fful<<40))
        return QString::number((bytes>>40) + ((bytes>>30) & (0x3fful)) / 1024.0, 'f', 2) + " TB";
    else if (bytes & (0x3fful<<30))
        return QString::number((bytes>>30) + ((bytes>>20) & (0x3fful)) / 1024.0, 'f', 2) + " GB";
    else if (bytes & (0x3fful<<20))
        return QString::number((bytes>>20) + ((bytes>>10) & (0x3fful)) / 1024.0, 'f', 2) + " MB";
    else if (bytes & (0x3fful<<10))
        return QString::number((bytes>>10) + ((bytes) & (0x3fful)) / 1024.0, 'f', 2) + " kB";
    else
        return QString::number(bytes, 'f', 0) + " B ";
}

class ViewContainer::Private
{
public:
    Private(ViewContainer *container) : q(container) {}
    ViewContainer * const q;
    bool back;
    ViewContainer::View currentView;
    FS::ProxyModel *model;
    QStackedLayout *viewStack;
    QItemSelectionModel *selectModel;
    KUrlNavigator *navigator;
    QVBoxLayout *layout;
    QAbstractItemView *view[ViewContainer::NViews];
    KFileItemActions *fileActions;
    KFileItem rootItem;
    QUrl startUrl;
};

ViewContainer::ViewContainer(QWidget *parent, KFilePlacesModel *placesModel, const QUrl &url)
    : QWidget(parent)
    , Configurable()
    , d(new Private(this))
{
    d->fileActions = new KFileItemActions(this);
    d->view[Icon] = new IconView(this);
    d->view[Details] = new DetailsView(this);
    d->view[Column] = new ColumnView(this);
    d->view[Flow] = new FlowView(this);
    d->model = new FS::ProxyModel(this);
    connect(static_cast<FS::DirLister *>(d->model->dirLister()), &FS::DirLister::started, this, [this](const QUrl &url)
    {
        if (url == rootUrl())
            setRootIndex(d->model->indexForUrl(url));
    });
    d->selectModel = new QItemSelectionModel(d->model);
    d->navigator = new KUrlNavigator(placesModel, url, this);
    connect(d->selectModel, &QItemSelectionModel::selectionChanged, this, &ViewContainer::selectionChanged);
//    connect(d->selectModel, &QItemSelectionModel::selectionChanged, this, [this]()
//    {
//        if (!d->selectModel->selectedIndexes().isEmpty())
//            d->selectModel->setCurrentIndex(d->selectModel->selectedIndexes().first(), QItemSelectionModel::NoUpdate);
//        else
//            d->selectModel->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
//    });
    connect(flowView(), &FlowView::iconSizeChanged, this, &ViewContainer::iconSizeChanged);
    connect(flowView()->flow(), &Flow::centerIndexChanged, this, &ViewContainer::entered);

    connect(iconView(), &IconView::newTabRequest, this, &ViewContainer::genNewTabRequest);
    connect(detailsView(), &DetailsView::newTabRequest, this, &ViewContainer::genNewTabRequest);
    connect(columnView(), &ColumnView::newTabRequest, this, &ViewContainer::genNewTabRequest);
    connect(flowView(), &FlowView::newTabRequest, this, &ViewContainer::genNewTabRequest);

    connect(d->navigator, &KUrlNavigator::urlChanged, this, &ViewContainer::slotUrlChanged);

    d->viewStack = new QStackedLayout();
    d->viewStack->setSpacing(0);
    d->viewStack->setContentsMargins(0,0,0,0);
    for (int i = 0; i < NViews; ++i)
    {
        connect(d->view[i], &QAbstractItemView::entered, this, &ViewContainer::entered);
        connect(d->view[i], SIGNAL(requestRightClickMenu(const QPoint &)), this, SLOT(slotMakeRightClickMenu(const QPoint &)));
        connect(d->view[i], SIGNAL(opened(const QModelIndex &)), this, SLOT(activate(const QModelIndex &)));
        d->viewStack->addWidget(d->view[i]);
        d->view[i]->setMouseTracking(true);
        d->view[i]->setModel(d->model);
        d->view[i]->setSelectionModel(d->selectModel);
    }
    d->layout = new QVBoxLayout();
    d->layout->setSpacing(0);
    d->layout->setContentsMargins(0,0,0,0);
    d->layout->addLayout(d->viewStack, 1);
    QFrame *frame = new QFrame();
    frame->setFrameStyle(QFrame::Sunken|QFrame::StyledPanel);
    QLayout *fl = new QHBoxLayout(frame);
    fl->setMargin(0);
    fl->addWidget(d->navigator);
    d->layout->addWidget(frame);
    connect(d->model, &FS::ProxyModel::urlLoaded, this, &ViewContainer::urlLoaded);
    connect(d->model, &FS::ProxyModel::urlItemsChanged, this, &ViewContainer::urlItemsChanged);

    setLayout(d->layout);
    setView(Icon, false);
    setRootUrl(url);
}

ViewContainer::~ViewContainer()
{
    delete d;
}

void
ViewContainer::reconfigure()
{
    KConfigGroup config = KSharedConfig::openConfig("NSEDocSurf.conf")->group("Views");
    setIconSize(config.readEntry("IconSize", 48/3)*16);
}

QString
ViewContainer::title() const
{
    QString title = rootUrl().fileName();
    if (title.isEmpty())
        title = rootUrl().path();
    if (title.isEmpty())
        title = rootUrl().scheme();
    if (title.isEmpty())
        title = rootUrl().toString();
    return title;
}

void
ViewContainer::slotMakeRightClickMenu(const QPoint &pt)
{
    ActionContainer *container = static_cast<MainWindow *>(window())->actionContainer();
    QMenu menu;
    if (rootUrl().scheme() == "trash")
    {
        menu.addActions(container->trashActions());
    }
    else
    {
        typedef QAction * (*Separator)(QObject *parent);
        static Separator separator = [] (QObject *parent) -> QAction *
        {
            QAction *a = new QAction(parent);
            a->setSeparator(true);
            return a;
        };
        KNewFileMenu *newFile = window()->findChild<KNewFileMenu *>();
        QList<QUrl> urls = selectedUrls();
        if (urls.isEmpty())
            urls << d->model->urlForIndex(currentView()->rootIndex());
        newFile->setPopupFiles(urls);
        newFile->checkUpToDate();
        QList<QAction *> firstGroup;
        firstGroup << container->action(ActionContainer::OpenInTab)
                   << newFile
                   << separator(&menu)
                   << container->action(ActionContainer::Paste)
                   << container->action(ActionContainer::Copy)
                   << container->action(ActionContainer::Cut)
                   << separator(&menu)
                   << container->action(ActionContainer::MoveToTrashAction)
                   << container->action(ActionContainer::DeleteSelection)
                   << container->action(ActionContainer::Rename);
        QList<QAction *> secondGroup;
        secondGroup << container->action(ActionContainer::GoUp)
                    << container->action(ActionContainer::GoBack)
                    << container->action(ActionContainer::GoForward)
                    << separator(&menu)
                    << container->action(ActionContainer::Properties);

        //            KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems
        //                                                              | KParts::BrowserExtension::ShowProperties
        //                                                              | KParts::BrowserExtension::ShowUrlOperations;

        menu.addActions(firstGroup);
        menu.addSeparator();
//        menu.addMenu(newFile->popupMenu());

        if (!selectedFiles().isEmpty())
        {
            d->fileActions->setItemListProperties(KFileItemListProperties(selectedFiles()));
            d->fileActions->addPluginActionsTo(&menu);
            d->fileActions->addServiceActionsTo(&menu);
            d->fileActions->addOpenWithActionsTo(&menu);
        }

        menu.addSeparator();
        menu.addActions(secondGroup);
    }
    menu.exec(pt);
}

void
ViewContainer::setView(const View view, bool store)
{
    d->currentView = view;
    d->viewStack->setCurrentWidget(d->view[view]);
    emit viewChanged();

//#if defined(ISUNIX)
//    if (Store::config.views.dirSettings && store && d->model->rootUrl().isLocalFile())
//        Ops::writeDesktopValue<int>(QDir(d->model->rootUrl().toLocalFile()), "view", (int)view);
//#endif
}



QModelIndexList
ViewContainer::selectedItems() const
{
    QModelIndexList selectedItems;
    if (selectionModel()->selectedRows(0).count())
        selectedItems = selectionModel()->selectedRows(0);
    else
        selectedItems = selectionModel()->selectedIndexes();
    return selectedItems;
}

QList<QUrl>
ViewContainer::selectedUrls() const
{
    QList<QUrl> urls;
    QModelIndexList idxs = selectedItems();
    for (int i = 0; i < idxs.count(); ++i)
        urls << idxs.at(i).data(FS::DirModel::FileItemRole).value<KFileItem>().url();
    return urls;
}

KFileItemList
ViewContainer::selectedFiles() const
{
    KFileItemList files;
    QModelIndexList idxs = selectedItems();
    for (int i = 0; i < idxs.count(); ++i)
        files << idxs.at(i).data(FS::DirModel::FileItemRole).value<KFileItem>();
    return files;
}

void
ViewContainer::deleteCurrentSelection()
{
//    if (DeleteDialog(selectedItems(), window()).result() == QDialog::Accepted)
//        KIO::del(selectedUrls());
    const QList<QUrl> list = KDirModel::simplifiedUrlList(selectedUrls());

    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(window());
    if (uiDelegate.askDeleteConfirmation(list, KIO::JobUiDelegate::Delete, KIO::JobUiDelegate::DefaultConfirmation))
    {
        KIO::DeleteJob *job = KIO::del(list);
        KJobWidgets::setWindow(job, window());
        connect(job, &KIO::DeleteJob::result, this, [this](KJob *job)
        {
            if (job->error())
                job->uiDelegate()->showErrorMessage();
        });
//        uiDelegate.setWindow(window());
    }
}

void
ViewContainer::activate(const QModelIndex &index)
{
    KFileItem item = d->model->itemForIndex(index);
    if (item.isNull())
        return;
    if (item.isDir())
        d->navigator->setLocationUrl(item.url());
    else
    {
        if (item.isDesktopFile())
        {
            KDesktopFile desktopItem(item.localPath());
            if (desktopItem.hasApplicationType()
                    && desktopItem.isAuthorizedDesktopFile(item.localPath()))
            {
                KService service(&desktopItem);
                KRun::runApplication(service, QList<QUrl>(), window());
            }
        }
        else
            KRun::runUrl(item.url(), item.mimetype(), window());
    }
}

void
ViewContainer::setRootIndex(const QModelIndex &index)
{
    for (int i = 0; i < NViews; ++i)
        d->view[i]->setRootIndex(index);
}

void
ViewContainer::slotUrlChanged(const QUrl &url)
{
    if (url == d->model->currentUrl())
        return;

    d->model->setCurrentUrl(url);
    iconView()->reset();
    columnView()->reset();
    flowView()->reset();
    detailsView()->reset();
    d->rootItem = url;
    emit urlChanged(url);
}

void
ViewContainer::setRootUrl(const QUrl &url)
{
    if (d->navigator->locationUrl(d->navigator->historyIndex()) != url)
        d->navigator->setLocationUrl(url);
    else
        slotUrlChanged(url);
}

QUrl
ViewContainer::rootUrl() const
{
    return d->model->currentUrl();
}

void
ViewContainer::genNewTabRequest(const QModelIndex &index)
{
    emit newTabRequest(index.data(FS::DirModel::FileItemRole).value<KFileItem>().url());
}

quint8
ViewContainer::currentViewAction() const
{
    return viewAction(d->currentView);
}

quint8
ViewContainer::viewAction(const View view)
{
    switch (view)
    {
    case ViewContainer::Icon: return ActionContainer::Views_Icon;
    case ViewContainer::Details: return ActionContainer::Views_Detail;
    case ViewContainer::Column: return ActionContainer::Views_Column;
    case ViewContainer::Flow: return ActionContainer::Views_Flow;
    default: return ActionContainer::Views_Icon;
    }
}

void ViewContainer::setIconSize(int stop)
{
    for (int i = 0; i < NViews; ++i)
    {
        d->view[i]->setIconSize(QSize(stop, stop));
        QList<QAbstractItemView *> kids(d->view[i]->findChildren<QAbstractItemView *>());
        for (int i = 0; i < kids.count(); ++i)
            kids.at(i)->setIconSize(QSize(stop, stop));
    }
}

void
ViewContainer::restoreFromTrash() const
{
    KIO::RestoreJob *job = KIO::restoreFromTrash(selectedUrls());
//    connect(job, &KIO::RestoreJob::finished, job, &KIO::RestoreJob::deleteLater); //jobs apparently delete themselves when finished...
}

void
ViewContainer::moveToTrash() const
{
    KIO::CopyJob *job = KIO::trash(selectedUrls());
//    connect(job, &KIO::CopyJob::finished, job, &KIO::CopyJob::deleteLater);
}

QAbstractItemView
*ViewContainer::currentView() const
{
    if (currentViewType() == Column)
        return static_cast<ColumnView *>(d->view[d->currentView])->activeColumn();
    return d->view[d->currentView];
}

bool ViewContainer::canGoBack()
{
    return d->navigator->historySize()>1 && (d->navigator->historyIndex() < d->navigator->historySize()-1);
}
bool ViewContainer::canGoForward() { return d->navigator->historyIndex(); }

QModelIndex ViewContainer::indexAt(const QPoint &p) const { return currentView()->indexAt(mapFromParent(p)); }
void ViewContainer::setPathVisible(bool visible) { d->navigator->setVisible(visible); }
bool ViewContainer::pathVisible() { return d->navigator->isVisible(); }
KUrlNavigator *ViewContainer::urlNav() { return d->navigator; }
void ViewContainer::setPathEditable(const bool editable) { d->navigator->setUrlEditable(editable); }
const QSize ViewContainer::iconSize() const { return d->view[Icon]->iconSize(); }
FS::ProxyModel *ViewContainer::model() { return d->model; }
QItemSelectionModel *ViewContainer::selectionModel() const { return d->selectModel; }
void ViewContainer::goBack() { d->navigator->goBack(); }
void ViewContainer::goForward() { d->navigator->goForward(); }
bool ViewContainer::goUp() { return d->navigator->goUp(); }
void ViewContainer::goHome() { d->navigator->setLocationUrl(QUrl::fromLocalFile(QDir::homePath())); }
void ViewContainer::refresh() { d->model->dirLister()->updateDirectory(d->model->currentUrl()); }
void ViewContainer::rename() { d->view[d->currentView]->edit(d->view[d->currentView]->currentIndex()); }
void ViewContainer::sort(const int column, const Qt::SortOrder order) { d->model->sort(column, order); }
const KFileItem &ViewContainer::rootItem() const { return d->rootItem; }
ViewContainer::View ViewContainer::currentViewType() const { return d->currentView; }

#define D_VIEW(_TYPE_, _METHOD_) _TYPE_##View *ViewContainer::_METHOD_##View() { return static_cast<_TYPE_##View *>(d->view[_TYPE_]); }
    D_VIEW(Icon, icon) D_VIEW(Details, details) D_VIEW(Column, column) D_VIEW(Flow, flow)
#undef D_VIEW
