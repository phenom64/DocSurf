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


#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <KDirSortFilterProxyModel>
#include <KDirLister>
#include <KDirModel>
#include "widgets.h"

#include <QSettings>
#include <QDir>

class QFileSystemWatcher;
class QMenu;

namespace KDFM
{

namespace FS
{

class DirLister : public KDirLister
{
    Q_OBJECT
public:
    DirLister(QObject *parent = 0);
    ~DirLister();

    void setShowingDotFiles(bool show);
    void updateDirectory(const QUrl &url);

protected:
    bool matchesFilter(const KFileItem &item) const;

private:
    QList<QRegExp> m_filter;
};

class DirModel;

class ProxyModel : public KDirSortFilterProxyModel , public Configurable
{
    Q_OBJECT
public:
    explicit ProxyModel(QObject *parent);

    QUrl urlForIndex(const QModelIndex &index) const;
    void setCurrentUrl(const QUrl &url);
    QUrl currentUrl() const;
    void count(int &dirs, int &files, qulonglong &bytes);

    QModelIndex indexForUrl(const QUrl &url) const;
    KFileItem itemForIndex(const QModelIndex &index) const;
    KDirLister *dirLister() const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    bool isRunning() const;
    QString nameFilter() const ;

    void reconfigure() override;

signals:
    void urlLoaded(const QUrl &url);
    void urlItemsChanged();

public slots:
    void slotFilterByName(const QString &filter);

private:
    DirModel *m_model;
    QString m_filter;
};

class PreviewLoader;
class DirModel : public KDirModel
{
    Q_OBJECT
public:
    explicit DirModel(QObject *parent = 0);
    ~DirModel();

    QUrl urlForIndex(const QModelIndex &index) const;
    void setCurrentUrl(const QUrl &url);
    QUrl currentUrl() const;
    static QList<QUrl> &tried() { return s_tried; }
    void count(int &dirs, int &files, qulonglong &bytes);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QStringList mimeTypes() const { return QStringList() << "text/uri-list" << "application/x-kde-ark-dndextract-service" << "application/x-kde-ark-dndextract-path"; }
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

protected slots:
    void slotPreviewLoaded(const KFileItem &file, const QPixmap &pix);

private:
    PreviewLoader *m_previewLoader;
    static QMap<QUrl, QPixmap> s_thumbs;
    static QList<QUrl> s_tried;
};

class PreviewLoader : public QObject, public Configurable
{
    Q_OBJECT
public:
    PreviewLoader(QObject *parent = 0);
    ~PreviewLoader();
    void reconfigure();
    void requestPreview(const KFileItem &file);

signals:
    void previewLoaded(const KFileItem &file, const QPixmap &pix);

protected slots:
    void loadPreviews();

private:
    QTimer *m_timer;
    KFileItemList m_queue;
    bool m_loadRemote;
    QStringList m_plugins;
};

template<typename T> static inline bool writeDesktopValue(const QDir &dir, const QString &key, T v, const QString &custom = QString())
{
    if (dir.isAbsolute() && QFileInfo(dir.path()).isWritable())
    {
        QSettings settings(dir.absoluteFilePath(".directory"), QSettings::IniFormat);
        settings.beginGroup("DFM");
        settings.setValue(key, v);
        settings.endGroup();
        return true;
    }
    QString newKey;
    if (dir.isAbsolute() && dir.exists())
        newKey = dir.path();
    else if (!custom.isEmpty())
        newKey = custom;
    else
        return false;
    newKey.replace("/", "_");
    QSettings settings("dfm", "desktopFile");
    settings.beginGroup(newKey);
    settings.setValue(key, v);
    settings.endGroup();
    return true;
}
template<typename T> static inline T getDesktopValue(const QDir &dir, const QString &key, bool *ok = 0, const QString &custom = QString())
{
    if (ok)
        *ok = false;
    if (!dir.isAbsolute() && custom.isEmpty())
        return T();
    const QFileInfo fi(dir.absoluteFilePath(".directory"));
    QVariant var;
    if (dir.isAbsolute() && fi.isReadable() && fi.isAbsolute())
    {
        QSettings settings(fi.filePath(), QSettings::IniFormat);
        settings.beginGroup("DFM");
        var = settings.value(key);
        settings.endGroup();
    }
    else
    {
        QString newKey;
        if (dir.isAbsolute() && dir.exists())
            newKey = dir.path();
        else if (!custom.isEmpty())
            newKey = custom;
        else
            return T();
        newKey.replace("/", "_");
        QSettings settings("dfm", "desktopFile");
        settings.beginGroup(newKey);
        var = settings.value(key);
        settings.endGroup();
    }
    if (var.isValid())
    {
        if (ok)
            *ok = true;
        return var.value<T>();
    }
    return T();
}

static void getSorting(const QString &file, int &sortCol, Qt::SortOrder &order)
{
    const QDir dir(file);
    QSettings settings(dir.absoluteFilePath(".directory"), QSettings::IniFormat);
    settings.beginGroup("DFM");
    QVariant varCol = settings.value("sortCol");
    QVariant varOrd = settings.value("sortOrd");
    if (varCol.isValid() && varOrd.isValid())
    {
        sortCol = varCol.value<int>();
        order = (Qt::SortOrder)varOrd.value<int>();
    }
    settings.endGroup();
}

}

}

#endif // FILESYSTEMMODEL_H
