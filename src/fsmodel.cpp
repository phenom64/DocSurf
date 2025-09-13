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

#include <QImageReader>
#include <QDirIterator>
#include <QDesktopServices>
#include <QFileSystemWatcher>
#include <QMessageBox>
#include <QPainter>
#include <QAbstractItemView>
#include <QDir>
#include <QTimer>
#include <QMimeData>
#include <QSettings>
#include <QDateTime>
#include <QUrl>
#include <QLabel>
#include <QDebug>
#include <QMap>
#include <QWaitCondition>
#include <QMenu>
#include <QString>
#include <QApplication>
#include <QProcess>
#include <QInputDialog>
#if !defined(QT_NO_DBUS)
#include <QDBusMessage>
#include <QDBusConnection>
#endif

#include <KF5/KIOCore/KIO/CopyJob>
#include <KF5/KIOFileWidgets/KAbstractViewAdapter>
#include <KF5/KIOFileWidgets/KFilePreviewGenerator>
#include <KF5/KIOCore/KIO/Job>
#include <KF5/KIOWidgets/KIO/PreviewJob>
#include <KF5/KIOWidgets/KAbstractFileItemActionPlugin>
#include <KF5/KConfigCore/KSharedConfig>
#include <KF5/KConfigCore/KConfigGroup>
#include <KF5/KIOCore/KFileItem>
#include <KF5/KIOCore/KCoreDirLister>
#include <KF5/KIOCore/KDirNotify>
#include <KF5/KIOCore/KIO/ListJob>
#include <KF5/KIOCore/KFileItemListProperties>

#include <KF5/KCoreAddons/KPluginLoader>
#include <KService/KService>
#include <KService/KServiceType>
#include <KParts/KParts/ReadOnlyPart>
#include <KParts/KParts/ReadWritePart>
#include <KParts/KParts/Part>
#include <KParts/Plugin>

#include "fsmodel.h"

using namespace DocSurf;
using namespace FS;

DirLister::DirLister(QObject *parent) : KDirLister(parent)
{

}

DirLister::~DirLister()
{

}

void
DirLister::updateDirectory(const QUrl &url)
{
    DirModel::tried().clear();
    KDirLister::updateDirectory(url);
}

void
DirLister::setShowingDotFiles(bool show)
{
    KDirLister::setShowingDotFiles(show);
    emitChanges();
}

bool
DirLister::matchesFilter(const KFileItem &item) const
{
    if (item.text() == QLatin1String(".."))
        return false;
    if (item.isHidden() && !showingDotFiles())
        return false;
    if (nameFilter().isEmpty())
        return true;
    const QStringList filters = nameFilter().split(" ", QString::SkipEmptyParts);
    for (int i = 0; i < filters.count(); ++i)
        if (item.name(true).contains(filters.at(i).toLower()))
            return true;
    return false;
}

ProxyModel::ProxyModel(QObject *parent)
    : KDirSortFilterProxyModel(parent)
    , Configurable()
    , m_model(new DirModel(parent))
{
    setSourceModel(m_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setCategorizedModel(true);
    setFilterWildcard("*");
    m_model->dirLister()->setAutoUpdate(true);

    connect(m_model->dirLister(), QOverload<const QUrl &>::of(&DirLister::completed), this, &ProxyModel::urlLoaded);
    connect(m_model->dirLister(), &DirLister::itemsAdded, this, &ProxyModel::urlItemsChanged, Qt::QueuedConnection);
    connect(m_model->dirLister(), &DirLister::itemsDeleted, this, &ProxyModel::urlItemsChanged, Qt::QueuedConnection);
    reconfigure();
}

void
ProxyModel::reconfigure()
{
    KConfigGroup config = KSharedConfig::openConfig("NSEDocSurf.conf")->group("Views");
    setCategorizedModel(config.readEntry("Categorized", false));
}

QVariant
ProxyModel::data(const QModelIndex &index, int role) const
{
    return KDirSortFilterProxyModel::data(index, role);
}

void
ProxyModel::setCurrentUrl(const QUrl &url)
{
    m_model->setCurrentUrl(url);
}

QUrl
ProxyModel::currentUrl() const
{
    return m_model->currentUrl();
}

QModelIndex
ProxyModel::indexForUrl(const QUrl &url) const
{
    return mapFromSource(m_model->indexForUrl(url));
}

KFileItem
ProxyModel::itemForIndex(const QModelIndex &index) const
{
    return m_model->itemForIndex(mapToSource(index));
}

QUrl
ProxyModel::urlForIndex(const QModelIndex &index) const
{
    return m_model->urlForIndex(mapToSource(index));
}

KDirLister
*ProxyModel::dirLister() const
{
    return m_model->dirLister();
}

bool
ProxyModel::isRunning() const
{
    return !dirLister()->isFinished();
}

void
ProxyModel::slotFilterByName(const QString &filter)
{
    m_filter = filter;
    emit layoutAboutToBeChanged();
    setFilterRegExp(filter);
//    setFilterFixedString(filter);
    emit layoutChanged();
//    dirLister()->setNameFilter(filter);
//    dirLister()->emitChanges();
//    dirLister()->openUrl(dirLister()->url());
}

QString
ProxyModel::nameFilter() const
{
    return m_filter;
//    return dirLister()->nameFilter();
}

void
ProxyModel::count(int &dirs, int &files, qulonglong &bytes)
{
    return static_cast<DirModel *>(sourceModel())->count(dirs, files, bytes);
}

QMap<QUrl, QPixmap> DirModel::s_thumbs;
QList<QUrl> DirModel::s_tried;

DirModel::DirModel(QObject *parent)
    : KDirModel(parent)
    , m_previewLoader(new PreviewLoader(this))
{
    setDirLister(new DirLister(this));
    setDropsAllowed(KDirModel::DropOnDirectory);
    connect(m_previewLoader, &PreviewLoader::previewLoaded, this, &DirModel::slotPreviewLoaded);
}

DirModel::~DirModel()
{
}

void
DirModel::count(int &dirs, int &files, qulonglong &bytes)
{
    dirs = 0;
    files = 0;
    bytes = 0;
    KFileItemList fileList = dirLister()->items();
    for (int i = 0; i < fileList.count(); ++i)
    {
        if (fileList.at(i).isDir())
            ++dirs;
        else if (fileList.at(i).isFile())
        {
            ++files;
            bytes += fileList.at(i).size();
        }
    }
}

void
DirModel::slotPreviewLoaded(const KFileItem &file, const QPixmap &pix)
{
    const QModelIndex &index = indexForItem(file);
    s_thumbs.insert(file.url(), pix);
    emit dataChanged(index, index, QVector<int>() << Qt::DecorationRole);
}

QVariant
DirModel::data(const QModelIndex &index, int role) const
{
    if (role == KDirSortFilterProxyModel::CategoryDisplayRole
            || role == KDirSortFilterProxyModel::CategorySortRole)
    {
        const KFileItem &item = itemForIndex(index);
        if (item.isDir())
            return role == KDirSortFilterProxyModel::CategorySortRole ? "0" : "directory";

//        if (item.isRegularFile())
//        {
//            QString suffix = item.name(true);

//            if (!suffix.isEmpty())
//            {
//                QStringList suffixes = suffix.split(".");
//                if (suffixes.count())
//                    suffix = suffixes.last();
//                return suffix;
//            }
//        }
        if (item.isMimeTypeKnown())
        {
            QStringList list = item.currentMimeType().name().split("/");
            if (list.count() == 2)
                return list.first();
        }
        if (!item.mimetype().isEmpty() && item.mimetype().contains("/"))
            return item.mimetype().split("/").first();
        return "file";
        //            qDebug() << item.name(true).at(0) << "CategoryDisplayRole";

    }
    if (role == Qt::DecorationRole && index.column() == 0)
    if (dirLister()->isFinished())
    {
        const KFileItem &item = itemForIndex(index);
        if (s_thumbs.contains(item.url()))
            return QIcon(s_thumbs.value(item.url()));
        if (!s_tried.contains(item.url()))
        {
            s_tried << item.url();
            m_previewLoader->requestPreview(item);
        }
    }
    return KDirModel::data(index, role);
}

QUrl
DirModel::urlForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return QUrl();
    KFileItem item = itemForIndex(index);
    if (!item.isNull())
        return item.url();
    return QUrl();
}

void
DirModel::setCurrentUrl(const QUrl &url)
{
    dirLister()->openUrl(url);
}

QUrl
DirModel::currentUrl() const
{
    return dirLister()->url();
}

bool
DirModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    KFileItem item = parent.data(FileItemRole).value<KFileItem>();
    if (!parent.isValid())
        item = currentUrl();
    if (!item.isDir())
        return false;
#if !defined(QT_NO_DBUS)
    //ark extract
    if (data->hasFormat("application/x-kde-ark-dndextract-service") && data->hasFormat("application/x-kde-ark-dndextract-path"))
    {
        const QString dBusPath(data->data("application/x-kde-ark-dndextract-path"));
        const QString dBusService(data->data("application/x-kde-ark-dndextract-service"));
        QDBusMessage message = QDBusMessage::createMethodCall(dBusService, dBusPath, "org.kde.ark.DndExtract", "extractSelectedFilesTo");
        message << item.localPath();
        QDBusConnection::sessionBus().send(message);
        return true;
    }
#endif
    if (data->urls().isEmpty())
        return false;
    QMenu menu;
    QAction *move = menu.addAction(QString("Move to %1").arg(item.name()));
    QAction *copy = menu.addAction(QString("Copy to %1").arg(item.name()));
    menu.addSeparator();

    QAction *extractAct = 0;
    QAction *extractActAuto = 0;
    QAction *extractActSub = 0;
    //this part seriously needs improvement... it uses extractfileitemaction only to test
    //if the file is an actual archive that ark can handle and then starts ark with the
    //commandline options accrodingly to extract the file...
    if (data->urls().count() == 1)
    {
        KFileItem arkitem(data->urls().first());
        KFileItemListProperties properties(KFileItemList() << arkitem);

        QString plugin = KPluginLoader::findPlugin("kf5/kfileitemaction/extractfileitemaction");
        KPluginLoader loader(plugin);
        QList<QAction *> actions;
        if (loader.load())
        {
            KAbstractFileItemActionPlugin *extractPlugin = loader.factory()->create<KAbstractFileItemActionPlugin>();
            actions << extractPlugin->actions(properties, 0);
            if (!actions.isEmpty())
            {
                extractAct = menu.addAction(QString("Extract to %1").arg(item.name()));
                extractActAuto = menu.addAction(QString("Extract to %1 (autodetect subfolder)").arg(item.name()));
                extractActSub = menu.addAction(QString("Extract to %1 (choose subfolder)").arg(item.name()));
            }
            extractPlugin->deleteLater();
        }
    }

    QAction *op = menu.exec(QCursor::pos());
    if (op == move)
        KIO::move(data->urls(), item.url());
    else if (op == copy)
        KIO::copy(data->urls(), item.url());
    else if (extractAct && op == extractAct)
        QProcess::startDetached("ark", QStringList() << "-b" << "-o" << item.localPath() << data->urls().first().toLocalFile());
    else if (extractAct && op == extractActAuto)
        QProcess::startDetached("ark", QStringList() << "-a" << "-b" << "-o" <<  item.localPath() << data->urls().first().toLocalFile());
    else if (extractAct && op == extractActSub)
    {
        QString folderName = QInputDialog::getText(qApp->activeWindow(), "Name for folder", "Foldername:");
        QDir(item.localPath()).mkdir(folderName);
        QProcess::startDetached("ark", QStringList() << "-b" << "-o" << QString("%1/%2").arg(item.localPath()).arg(folderName) << data->urls().first().toLocalFile());
    }
    else
        return false;
    return true;
}

PreviewLoader::PreviewLoader(QObject *parent)
    : QObject(parent)
    , Configurable()
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(100);
    connect(m_timer, &QTimer::timeout, this, &PreviewLoader::loadPreviews);
    reconfigure();
}

PreviewLoader::~PreviewLoader()
{
}

void
PreviewLoader::reconfigure()
{
    KConfigGroup config = KSharedConfig::openConfig("NSEDocSurf.conf")->group("Views");
    m_plugins = config.readEntry("PreviewPlugins", KIO::PreviewJob::defaultPlugins());
    m_loadRemote = config.readEntry("RemotePreviews", false);
    m_queue.clear();
    DirModel::tried().clear();
}

void
PreviewLoader::requestPreview(const KFileItem &file)
{
    if (m_queue.contains(file) || (file.isSlow() && !m_loadRemote) || file.isDir())
        return;
    m_queue << file;
//    if (!m_timer->isActive())
        m_timer->start();
}

void
PreviewLoader::loadPreviews()
{
//    qDebug() << "loading previews for" << m_queue.size() << "files...";
    KIO::PreviewJob *job = KIO::filePreview(m_queue, QSize(256, 256), &m_plugins);
    connect(job, &KIO::PreviewJob::gotPreview, this, &PreviewLoader::previewLoaded);
//    connect(job, &KIO::PreviewJob::finished, job, &QObject::deleteLater); //jobs delete themselves when finished
    m_queue.clear();
    m_timer->stop();
}
