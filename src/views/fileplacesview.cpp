
#include "fileplacesview.h"
#include "../gfx/color.h"
#include <KFilePlacesModel>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>
#include <QScrollBar>
#include <QPainter>
#include <QAction>
#include <QMap>
#include <QRect>
#include <QModelIndex>
#include <QTimer>

using namespace DocSurf;

class FilePlacesView::Private
{
public:
    Private(FilePlacesView *v)
        : q(v)
//        , contentsHeight(0)
//        , itemHeight(0)
//        , titleSize(32)
//        , showAll(false)
        , model(new KFilePlacesModel(v))
    {
        v->setModel(model);
    }
    FilePlacesView * const q;
//    int contentsHeight;
//    int itemHeight;
//    int titleSize;
//    bool showAll;
//    QRect placesRect, devicesRect;
    QModelIndex pressedIndex;
    KFilePlacesModel *model;
//    QMap<QModelIndex, QRect> layout;
//    void updateScrollBarPos()
//    {
//        if (contentsHeight > q->viewport()->height())
//            q->verticalScrollBar()->setRange(0, contentsHeight-q->viewport()->height());
//        else
//            q->verticalScrollBar()->setRange(0, -1);
//        q->verticalScrollBar()->setSingleStep(itemHeight);
//        q->verticalScrollBar()->setPageStep(q->viewport()->height());
//        q->verticalScrollBar()->setVisible(q->verticalScrollBar()->maximum() != -1);
//    }
};

FilePlacesView::FilePlacesView(QWidget *parent)
    : KFilePlacesView(parent)
    , d(new Private(this))
{
    setFrameStyle(QFrame::StyledPanel|QFrame::Sunken);
    QWidget *v = viewport();
    v->setForegroundRole(QPalette::Text);
    v->setBackgroundRole(QPalette::Base);
    v->setAutoFillBackground(true);
    v->setPalette(qApp->palette());
//    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](){viewport()->update();});
//    setSelectionMode(QAbstractItemView::SingleSelection);
    setViewMode(QListView::ListMode);
//    verticalScrollBar()->setAttribute(Qt::WA_OpaquePaintEvent);
//    horizontalScrollBar()->setAttribute(Qt::WA_OpaquePaintEvent);
}

FilePlacesView::~FilePlacesView()
{
    delete d;
}

void
FilePlacesView::mousePressEvent(QMouseEvent *e)
{
    KFilePlacesView::mousePressEvent(e);
    if (e->button() == Qt::MidButton)
    {
        blockSignals(true);
        d->pressedIndex = indexAt(e->pos());
    }
    else
        d->pressedIndex = QModelIndex(); 
}

void
FilePlacesView::mouseReleaseEvent(QMouseEvent *e)
{
    KFilePlacesView::mouseReleaseEvent(e);
    blockSignals(false);
    if (e->button() == Qt::MidButton && d->pressedIndex == indexAt(e->pos()))
    {
        emit newTabRequest(d->model->url(d->pressedIndex));
    }
    d->pressedIndex = QModelIndex();
}

QModelIndex
FilePlacesView::indexAt(const QPoint &p) const
{
    for (int i = 0; i < model()->rowCount(rootIndex()); ++i)
    {
        const QModelIndex &index = d->model->index(i, 0, rootIndex());
        if (d->model->isHidden(index))
            continue;
        if (visualRect(index).contains(p))
            return index;
    }
    return QModelIndex();
}
