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
#include <QWheelEvent>
#include <QFileInfo>
#include <QStyleOptionGraphicsItem>
#include <QCoreApplication>
#include <QBitmap>
#include <QElapsedTimer>
#include <QApplication>
#include <qmath.h>
#include <QTransform>
#include <QRegion>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsItemAnimation>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsProxyWidget>
#include <QGraphicsEffect>
#include <QItemSelectionModel>
#include <QHash>
#include <QMouseEvent>
#include <QGLWidget>
#include <QList>
#include <QTimeLine>
#include <QDebug>
#include <QScrollBar>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QQueue>

#include <KFileItem>

#include "gfx/color.h"
#include "flow.h"
#include "fsmodel.h"

using namespace DocSurf;

#define ANGLE 66.0f
#define SCALE 0.80f
#define PERSPECTIVE 0.6f
#define TESTPERF1 QElapsedTimer timer; timer.start()
#define TESTPERF2(_TEXT_) qDebug() << _TEXT_ << timer.elapsed() << "millisecs"
#define SIZE 258.0f
#define RECT QRectF(0.0f, 0.0f, SIZE, SIZE)

static float space = 48.0f, bMargin = 8;

class ScrollBar : public QScrollBar
{
public:
    QPixmap slider[2], groove;
    explicit ScrollBar(const Qt::Orientation o, QWidget *parent = 0) : QScrollBar(o, parent) {}
    void setRange(int min, int max) { clearCache(); QScrollBar::setRange(min, max); }
    void resizeEvent(QResizeEvent *e) { clearCache(); QScrollBar::resizeEvent(e); }
    void clearCache() { for (int i = 0; i < 2; ++i) slider[i] = QPixmap(); groove = QPixmap(); }
protected:
    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        QStyleOptionSlider opt;
        initStyleOption(&opt);

        const QRect &grooveRect = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarGroove, this);
        const QRect &sliderRect = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this).adjusted(1, 1, -1, -1);

        if (grooveRect.isEmpty() || sliderRect.isEmpty())
            return;
        // we need to paint to pixmaps first to get antialiasing...

        if (groove.isNull())
        {
            //groove
            const int h = palette().color(QPalette::Highlight).hue();
            QColor c; c.setHsv(h, 128, 32, 192);
            QPixmap grvPix(grooveRect.size());
            grvPix.fill(Qt::transparent);
            const int grvR = qCeil(grvPix.height()/2.0f);
            QPainter grvPt(&grvPix);

            grvPt.setRenderHint(QPainter::Antialiasing);
            grvPt.setPen(Qt::NoPen);
            grvPt.setBrush(c);
            grvPt.drawRoundedRect(grvPix.rect(), grvR, grvR);
            grvPt.end();
            groove = grvPix;

            //slider
            c.setHsv(h, 32, 222);
            for (int i = 0; i < 2; ++i)
            {
                c.setAlpha(i?128:64);
                QPixmap sldrPix(sliderRect.size());
                sldrPix.fill(Qt::transparent);
                const int sldrR = qCeil(sldrPix.height()/2.0f);
                QPainter sldrPt(&sldrPix);
                sldrPt.setRenderHint(QPainter::Antialiasing);
                sldrPt.setPen(Qt::NoPen);
                sldrPt.setBrush(c);
                sldrPt.drawRoundedRect(sldrPix.rect(), sldrR, sldrR);
                sldrPt.end();
                slider[i] = sldrPix;
            }
        }
        p.drawTiledPixmap(grooveRect, groove);
        p.drawTiledPixmap(sliderRect, slider[underMouse()]);
        p.end();
    }
};

class GraphicsScene : public QGraphicsScene
{
public:
    Flow *preView;
    QBrush fgBrush, bgBrush;
    inline explicit GraphicsScene(const QRectF &rect = QRectF(), Flow *flow = 0)
        : QGraphicsScene(rect, flow)
        , preView(flow) {}
    ~GraphicsScene() {}

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) { painter->fillRect(rect, bgBrush); }
    void drawForeground(QPainter *painter, const QRectF &rect) { painter->fillRect(rect, fgBrush); }
};

class Flow::RootItem : public QGraphicsPixmapItem
{
public:
    explicit RootItem(QGraphicsScene *scene = 0) : QGraphicsPixmapItem() { scene->addItem(this); }
    ~RootItem(){}
    QRectF boundingRect() { return scene()->sceneRect(); }
};

class Flow::Item : public QGraphicsItem
{
    friend class Flow;
public:
    QPixmap pix[2];
    GraphicsScene *scene;
    Flow *preView;
    float rotate, savedX;
    bool dirty;
    QPainterPath path;
    Item(GraphicsScene *scene, QGraphicsItem *parent)
        : QGraphicsItem(parent)
        , scene(scene)
        , dirty(true)
    {
        this->preView = scene->preView;
        setY(preView->y());
        setTransformOriginPoint(boundingRect().center());
    }
    ~Item()
    {
        if (preView->items().contains(this))
            preView->items().removeOne(this);
    }
    void transform(const float angle, const Qt::Axis axis, const float xscale = 1.0f, const float yscale = 1.0f)
    {
        QTransform t;
        t.translate(SIZE/2.0f, SIZE*PERSPECTIVE);
        t.rotate(angle, axis);
        t.translate(-SIZE/2.0f, -SIZE*PERSPECTIVE);
        t.translate(SIZE/2.0f, SIZE/2.0f);
        t.scale(1.0f/*+(1.0f-xscale)/2*/, yscale);
        t.translate(-SIZE/2.0f, -SIZE/2.0f);
        setTransform(t);
        rotate = angle;
    }
    QRectF boundingRect() const { return RECT; }
    void saveX() { savedX = pos().x(); }
    QPainterPath shape() const { return path; }
    QModelIndex index() { return preView->indexOfItem(this); }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        if (dirty)
            updateIcon();
        if (painter->transform().isScaling())
            painter->setRenderHints(QPainter::SmoothPixmapTransform);
    //    painter->drawTiledPixmap(QRect(0, SIZE, SIZE, SIZE), m_pix[1]);

        const QRect rect(1,1,256,256);
        const QRect &pixRect = QApplication::style()->itemPixmapRect(rect, Qt::AlignBottom|Qt::AlignHCenter, pix[0]);
        painter->drawPixmap(pixRect, pix[0]);
        const QRect &refRect = QApplication::style()->itemPixmapRect(rect.translated(0, 258), Qt::AlignTop|Qt::AlignHCenter, pix[0]);
        painter->drawPixmap(refRect, pix[1]);
        painter->setRenderHints(QPainter::SmoothPixmapTransform, false);
    }
    void updateIcon()
    {
        pix[0] = index().data(Qt::DecorationRole).value<QIcon>().pixmap(QSize(256, 256));
        pix[1] = pix[0].transformed(QTransform().scale(1, -1));
        QPainter p(&pix[1]);
        static QColor bg = preView->bg();
        if (bg.alpha() == 0xff)
            bg.setAlpha(222);
        p.fillRect(pix[1].rect(), bg);
        p.end();
        updateShape();
        dirty = false;
    }
    void updateShape()
    {
        if (pix[0].isNull())
            return;

        const QRect rect(1,1,256,256);
        const QRect &pixRect = QApplication::style()->itemPixmapRect(rect, Qt::AlignBottom|Qt::AlignHCenter, pix[0]);
        QPainterPath p;
        p.addRegion(pixRect);
        path = p;
    }
};

class Flow::Private
{
public:
    Private(Flow *flow)
        : q(flow)
        , scene(new GraphicsScene(q->rect(), q))
        , model(0)
        , row(-1)
        , nextRow(-1)
        , newRow(-1)
        , savedRow(-1)
        , pressed(0)
        , y(0.0f)
        , x(0.0f)
        , timeLine(new QTimeLine(250, q))
        , scrollBar(0)
        , rootItem(new Flow::RootItem(scene))
        , wantsDrag(false)
        , perception(0.0f)
        , hasZUpdate(false)
        , xpos(0.0f)
    {}
    Flow * const q;
    QColor bg;
    GraphicsScene *scene;
    FS::ProxyModel *model;
    QModelIndex centerIndex, prevCenter, savedCenter;
    QPersistentModelIndex rootIndex;
    int row, nextRow, newRow, savedRow, sortColumn;
    Qt::SortOrder sortOrder;
    float y, x, perception, xpos;
    bool wantsDrag, hasZUpdate;
    QList<Flow::Item *> items;
    QGraphicsItemAnimation *anim[2];
    QTimeLine *timeLine;
    QGraphicsItem *pressed;
    QGraphicsSimpleTextItem *textItem;
    Flow::RootItem *rootItem;
    QGraphicsProxyWidget *gfxProxy;
    ScrollBar *scrollBar;
    QPointF pressPos;
    QItemSelectionModel *selectionModel;
    QUrl rootUrl, centerUrl;
    bool isValidRow(const int row) { return bool(row > -1 && row < items.count()); }
    int validate(const int row) const { return qBound(0, row, items.count()-1); }
    void populate(const int start, const int end)
    {
        for (int i = start; i <= end; i++)
            items.insert(i, new Flow::Item(scene, rootItem));

        QModelIndex index;
        if (centerUrl.isValid())
            index = model->indexForUrl(centerUrl);
        if (!index.isValid())
            index = model->index(validate(savedRow), 0, rootIndex);

        q->setCenterIndex(index);
        if (!items.isEmpty())
            q->setCenterIndex(model->index(0, 0, rootIndex));
        q->updateItemsPos();
        q->update();
    }
};

Flow::Flow(QWidget *parent)
    : QGraphicsView(parent)
    , d(new Private(this))
{
    QGLFormat glf = QGLFormat::defaultFormat();
    glf.setSampleBuffers(false);
    glf.setSwapInterval(0);
    glf.setStencil(true);
    glf.setAccum(true);
    glf.setDirectRendering(true);
    glf.setDoubleBuffer(true);
    QGLFormat::setDefaultFormat(glf);
    setMaximumHeight(SIZE*2.0f);
    QGLWidget *glWidget = new QGLWidget(glf, this);
    connect(qApp, &QApplication::aboutToQuit, glWidget, &QGLWidget::deleteLater);
    setViewport(glWidget);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setOptimizationFlag(QGraphicsView::DontSavePainterState);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing);
    setCacheMode(QGraphicsView::CacheBackground);
    d->scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    setScene(d->scene);
    d->textItem = new QGraphicsSimpleTextItem();
    ScrollBar *scrollBar = new ScrollBar(Qt::Horizontal);
    scrollBar->setSingleStep(1);
    scrollBar->setPageStep(1);
    d->gfxProxy = d->scene->addWidget(scrollBar);
    d->scrollBar = static_cast<ScrollBar *>(d->gfxProxy->widget());
    d->scrollBar->setAttribute(Qt::WA_NoSystemBackground);
    connect(d->scrollBar, &QScrollBar::valueChanged, this, &Flow::scrollBarMoved);
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::NoFrame);
    for (int i = 0; i < 2; ++i)
    {
        d->anim[i] = new QGraphicsItemAnimation(this);
        d->anim[i]->setTimeLine(d->timeLine);
    }
    d->timeLine->setEasingCurve(QEasingCurve::Linear);
    d->timeLine->setUpdateInterval(17); //17 ~ 60 fps
    connect(d->timeLine, &QTimeLine::valueChanged, this, &Flow::animStep);
    connect(d->timeLine, &QTimeLine::finished, this, &Flow::continueIf);

    d->textItem = new QGraphicsSimpleTextItem();
    d->scene->addItem(d->textItem);
    QFont f = font();
    f.setPointSize(16);
    f.setBold(true);
    d->textItem->setFont(f);
    d->textItem->setBrush(Qt::white);
    QGraphicsDropShadowEffect *dse = new QGraphicsDropShadowEffect(this);
    dse->setBlurRadius(4);
    dse->setOffset(0);
    dse->setColor(Qt::black);
    d->textItem->setGraphicsEffect(dse);
    setMouseTracking(false);
    setAttribute(Qt::WA_Hover, false);
    setCursor(Qt::ArrowCursor);

    d->bg = Color::mid(palette().color(QPalette::Highlight), Qt::black);
    d->bg.setHsv(d->bg.hue(), qMin(64, d->bg.saturation()), d->bg.value(), d->bg.alpha());
    d->scene->bgBrush = d->bg;
}

Flow::~Flow()
{
//    m_dataLoader->discontinue();
//    m_dataLoader->wait();
    qDeleteAll(d->items);
    d->items.clear();
    delete d;
}

QModelIndex
Flow::indexOfItem(Item *item) const
{
    if (d->items.contains(item))
        return d->model->index(d->items.indexOf(item), 0, d->rootIndex);
    return QModelIndex();
}

void
Flow::showNext()
{
    d->nextRow = d->row+1;
    prepareAnimation();
}

void
Flow::showPrevious()
{
    d->nextRow = d->row-1;
    prepareAnimation();
}

void
Flow::prepareAnimation()
{
    if (!d->isValidRow(d->newRow))
        return;

    int i = d->items.count();
    while (--i > -1)
        d->items.at(i)->saveX();
#define CENTER QPoint(d->x - SIZE / 2.0f, d->y)
#define LEFT QPointF((d->x - SIZE) - space, d->y)
#define RIGHT QPointF(d->x + space, d->y)
    d->anim[New]->setItem(d->items.at(d->validate(d->nextRow)));
    d->anim[New]->setPosAt(1, CENTER);
    d->anim[Prev]->setItem(d->items.at(d->validate(d->row)));
    d->anim[Prev]->setPosAt(1, d->nextRow > d->row ? LEFT : RIGHT);
#undef CENTER
#undef RIGHT
#undef LEFT
    d->hasZUpdate = false;
    d->timeLine->setDuration(qMax(1.0f, 250.0f / qAbs(d->row - d->newRow)));
    d->timeLine->start();
}

void
Flow::animStep(const qreal value)
{
    if (d->items.isEmpty())
        return;

    const float f = SCALE+(value*(1.0f-SCALE)), s = space*value;
    const bool goingUp = d->nextRow > d->row;

    float rotate = ANGLE * value;
    d->items.at(d->row)->transform(goingUp ? -rotate : rotate, Qt::YAxis, SCALE/f, SCALE/f);

    rotate = ANGLE-rotate;
    d->items.at(d->nextRow)->transform(goingUp ? rotate : -rotate, Qt::YAxis, f, f);

#define UP d->items.at(i)->savedX-s
#define DOWN d->items.at(i)->savedX+s

    int i = d->items.count();
    while (--i > -1)
    {
        if (i != d->row && i != d->nextRow)
            d->items.at(i)->setX(goingUp ? UP : DOWN);
        if (!d->hasZUpdate)
            d->items.at(i)->setZValue(i>=d->nextRow ? d->items.at(qMin(d->items.count()-1, i+1))->zValue()+1 : d->items.at(qMax(0, i-1))->zValue()-1);
    }

    d->hasZUpdate = true;

    if (value == 1)
    {
        setCenterIndex(d->model->index(d->nextRow, 0, d->rootIndex));
        if (d->newRow == d->row)
        {
            d->newRow = -1;
            d->nextRow = -1;
            emit centerIndexChanged(d->centerIndex);
        }
    }
}

void
Flow::continueIf()
{
    if (d->newRow > d->row)
        showNext();
    else if (d->newRow < d->row)
        showPrevious();
}

void
Flow::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        if (event->angleDelta().y() > 0)
            d->perception += 1;
        else
            d->perception -= 1;
        const float &y = d->y+SIZE;
        d->rootItem->setTransform(QTransform().translate(rect().width()/2.0f, y).rotate(d->perception, Qt::XAxis).translate(-rect().width()/2.0f, -y));
    }
    else if (event->modifiers() & Qt::MetaModifier)
        d->rootItem->setScale(d->rootItem->scale()+((float)event->angleDelta().y()*0.001f));
    else
        d->scrollBar->setValue(d->scrollBar->value()+(event->angleDelta().y()>0?-1:1));
//        QCoreApplication::sendEvent(d->scrollBar, event);
}

void
Flow::enterEvent(QEvent *e)
{
    QGraphicsView::enterEvent(e);
    if (QApplication::overrideCursor())
        QApplication::restoreOverrideCursor();
}

void
Flow::setModel(FS::ProxyModel *model)
{
    d->model = model;
    d->sortColumn = d->model->sortColumn();
    d->sortOrder = d->model->sortOrder();
    connect(d->model, &QAbstractItemModel::dataChanged, this, &Flow::dataChanged);
    connect(d->model, &QAbstractItemModel::modelAboutToBeReset, this, &Flow::clear);
    connect(d->model, &QAbstractItemModel::modelReset, this, &Flow::reset);
    connect(d->model, &QAbstractItemModel::rowsInserted, this, &Flow::rowsInserted);
    connect(d->model, &QAbstractItemModel::rowsRemoved, this, &Flow::rowsRemoved);
    connect(d->model, &QAbstractItemModel::layoutChanged, this, [this]()
    {
        if (d->sortColumn != d->model->sortColumn()
                || d->sortOrder != d->model->sortOrder())
        {
            d->sortColumn = d->model->sortColumn();
            d->sortOrder = d->model->sortOrder();
//            reset();
            for (int i = 0; i < d->items.count(); ++i)
                d->items.at(i)->dirty = true;
        }
    });
}

void
Flow::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!topLeft.isValid() || !bottomRight.isValid())
        return;

    const int start = topLeft.row();
    const int end = bottomRight.row();
    if (d->items.count() > end)
        for (int i = start; i <= end; ++i)
        {
            Item *item = d->items.at(i);
            item->dirty = true;
            item->update();
        }
}

void
Flow::setCenterIndex(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;

    QModelIndex index(idx);

    if (index.column())
        index = index.sibling(index.row(), 0);

    if (index.row())
    {
        d->savedRow = index.row();
        d->savedCenter = index;

    }
    else if (d->items.count() <= 1)
    {
        d->savedRow = 0;
        d->nextRow = 0;
        d->row = 0;
    }
    d->centerUrl = d->model->urlForIndex(index);
    d->prevCenter = d->centerIndex;
    d->centerIndex = index;
    d->nextRow = d->row;
    d->row = qMin(index.row(), d->items.count()-1);
    d->textItem->setText(index.data().toString());
    d->textItem->setZValue(d->items.count()+2);
    d->gfxProxy->setZValue(d->items.count()+2);
    d->textItem->setPos(d->x - d->textItem->boundingRect().width()/2.0f, rect().bottom() - (bMargin+d->scrollBar->height()+d->textItem->boundingRect().height()));
}

void
Flow::reset()
{
    clear();
    if (d->model && d->model->rowCount(d->rootIndex))
    {
        d->populate(0, d->model->rowCount(d->rootIndex)-1);
        d->scrollBar->setRange(0, d->items.count()-1);
        d->scrollBar->setValue(qBound(0, d->savedRow, d->items.count()-1));
    }
}

void
Flow::updateScene()
{
    if (!d->textItem)
        return;

    QModelIndex center = d->model->indexForUrl(d->centerUrl);
    if (!center.isValid())
        center = d->model->index(0, 0, d->rootIndex);

    if (d->textItem->text().isEmpty())
        d->textItem->setText(center.data().toString());
    d->scene->setSceneRect(QRect(QPoint(0, 0), size()));
    const float bottom = bMargin+d->scrollBar->height()+d->textItem->boundingRect().height();
    d->y = (height()/2.0f-SIZE/2.0f)-bottom;
    d->x = width()/2.0f;
    d->rootItem->update();
    updateItemsPos();
    d->scrollBar->resize(width()*0.66f, d->scrollBar->height());
    const float y = d->y+SIZE;
    const float scale = qMin<float>(1.0f, ((float)height()/SIZE)*0.8);
    d->textItem->setPos(qMax<float>(0.0f, d->x-d->textItem->boundingRect().width()/2.0f), rect().bottom()-(bMargin+d->scrollBar->height()+d->textItem->boundingRect().height()));
    d->textItem->setZValue(d->items.count()+2);
    d->gfxProxy->setPos(qRound(d->x - d->gfxProxy->boundingRect().width()/2.0f), qRound(rect().bottom()-(bMargin+d->scrollBar->height())));
    d->rootItem->setTransformOriginPoint(rect().center());
    d->rootItem->setTransform(QTransform().translate(rect().width()/2.0f, y).rotate(d->perception, Qt::XAxis).translate(-rect().width()/2.0f, -y));
    d->rootItem->setScale(scale);

    QRadialGradient rg(QPoint(rect().width()/2.0f, rect().bottom()*0.75f), rect().width()/2.0f);
    rg.setColorAt(0, Qt::transparent);
    rg.setColorAt(0.5, Qt::transparent);
    rg.setColorAt(0.75, QColor(0,0,0,64));
    rg.setColorAt(1, QColor(0,0,0,192));
    d->scene->fgBrush = rg;
}

void
Flow::rowsRemoved(const QModelIndex &parent, int start, int end)
{
    if (parent != d->rootIndex)
        return;

    if (d->items.isEmpty())
        return;

    d->timeLine->stop();

    for (int i = end; i >= start; --i)
        if (i < d->items.size())
            delete d->items.takeAt(i);

    d->scrollBar->blockSignals(true);
    d->scrollBar->setRange(0, d->items.count()-1);
    d->scrollBar->setValue(d->validate(start-1));
    d->scrollBar->blockSignals(false);
    QModelIndex center = d->model->index(d->validate(start-1), 0, d->rootIndex);
    if (d->items.count() == 1)
        center = d->model->index(0, 0, parent);
    setCenterIndex(center);
    updateItemsPos();
}

void
Flow::setRootIndex(const QModelIndex &rootIndex)
{
    clear();
    d->rootIndex = rootIndex;
    d->rootUrl = d->model->urlForIndex(rootIndex);
    d->savedRow = 0;
    d->savedCenter = QModelIndex();
    d->centerUrl = QUrl();
    reset();
}

void
Flow::rowsInserted(const QModelIndex &parent, int start, int end)
{
    if (parent != d->rootIndex)
        return;

    d->populate(start, end);

    if (!d->items.isEmpty())
        d->scrollBar->setRange(0, d->items.count()-1);
}

void
Flow::updateItemsPos()
{
    if (d->items.isEmpty()
         || d->row == -1
         || d->row > d->model->rowCount(d->rootIndex)-1
         || !isVisible())
        return;

    d->timeLine->stop();
    Item *center = d->items.at(d->row);
    center->setZValue(d->items.count());
    center->setPos(d->x-SIZE/2.0f, d->y);
    center->resetTransform();

    if (d->items.count() > 1)
        correctItemsPos(d->row-1, d->row+1);
}

void
Flow::correctItemsPos(const int leftStart, const int rightStart)
{
    int z = d->items.count()-1;
    Item *p = 0;
    QList<Item *>::iterator i = d->items.begin()+rightStart;
    if (i < d->items.end())
    {
        p = *i;
        p->setPos(rect().center().x()+space, d->y);
        p->transform(ANGLE, Qt::YAxis, SCALE, SCALE);
        d->xpos = p->x();
        p->setZValue(z);
        while (++i < d->items.end()) //right side
        {
            --z;
            p = *i;
            p->setPos(d->xpos+=space, d->y);
            p->transform(ANGLE, Qt::YAxis, SCALE, SCALE);
            p->setZValue(z);
        }
    }
    d->xpos = 0.0f;
    z = d->items.count()-1;
    i = d->items.begin()+leftStart;
    if (i > d->items.begin()-1)
    {
        p = *i;
        p->setPos((rect().center().x()-SIZE)-space, d->y);
        p->transform(-ANGLE, Qt::YAxis, SCALE, SCALE);
        d->xpos = p->x();
        p->setZValue(z);

        while (--i > d->items.begin()-1) //left side
        {
            --z;
            p = *i;
            p->setPos(d->xpos-=space, d->y);
            p->transform(-ANGLE, Qt::YAxis, SCALE, SCALE);
            p->setZValue(z);
        }
    }
    p = 0;
    d->xpos = 0.0f;
}


void
Flow::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    d->pressed = itemAt(event->pos());
    if (!d->pressed)
    {
        d->wantsDrag = true;
        d->pressPos = event->pos();
    }
}

void
Flow::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
    if (d->pressed && itemAt(event->pos()) == d->pressed)
    {
        const QModelIndex &index = indexOfItem(static_cast<Item *>(d->pressed));
        if (index.isValid())
        {
            if (index != d->centerIndex)
            {
                d->scrollBar->setValue(index.row());
            }
        }
    }
    d->wantsDrag = false;
}

void
Flow::mouseDoubleClickEvent(QMouseEvent *event)
{
    QGraphicsView::mouseDoubleClickEvent(event);
    if (d->pressed && itemAt(event->pos()) == d->pressed)
    {
        const QModelIndex &index = indexOfItem(static_cast<Item *>(d->pressed));
        if (index.isValid() && index == d->centerIndex)
        {
            emit opened(index);
        }
    }
}

void
Flow::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    if (!d->wantsDrag)
        return;
    if (event->pos().y() < d->pressPos.y())
        d->perception -= qAbs(event->pos().y() - d->pressPos.y())*0.1;
    else
        d->perception += qAbs(event->pos().y() - d->pressPos.y())*0.1;
    float y = d->y+SIZE;
    d->rootItem->setTransform(QTransform().translate(rect().width()/2.0f, y).rotate(d->perception, Qt::XAxis).translate(-rect().width()/2.0f, -y));
    d->pressPos = event->pos();
}

void
Flow::showCenterIndex(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    if (!isVisible()) //not visible... we silently update the index w/o animations
    {
        setCenterIndex(index);
        updateItemsPos();
        return;
    }

    d->newRow = index.row();

    if (d->newRow == d->row)
        return;

    if (qAbs(d->newRow-d->row) > 10)
    {
        int i = d->newRow > d->row ? -10 : +10;
        d->timeLine->stop();
        setCenterIndex(d->model->index(d->newRow+i, 0, d->rootIndex));
        updateItemsPos();
        showCenterIndex(d->model->index(d->newRow, 0, d->rootIndex));
        return;
    }
    if (d->timeLine->state() & QTimeLine::Running)
        return;
    if (d->newRow > d->row)
        showNext();
    else if (d->newRow < d->row)
        showPrevious();
}

void
Flow::clear()
{
    d->timeLine->stop();
    d->centerIndex = QModelIndex();
    d->prevCenter = QModelIndex();
    d->row = -1;
    d->nextRow = -1;
    d->newRow = -1;
    d->pressed = 0;
    d->savedRow = -1;
    d->savedCenter = QModelIndex();
    d->centerUrl = QUrl();
    d->savedRow = 0;
    qDeleteAll(d->items);
    d->textItem->setText(QString("--"));
    d->scrollBar->setValue(0);
    d->scrollBar->setRange(0, 0);
}

void
Flow::scrollBarMoved(const int value)
{
    if (d->items.count())
        showCenterIndex(d->model->index(qBound(0, value, d->items.count()), 0, d->rootIndex));
}

void
Flow::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    updateScene();
}

void
Flow::showEvent(QShowEvent *event)
{
    QGraphicsView::showEvent(event);
    updateScene();
}

void
Flow::setSelectionModel(QItemSelectionModel *model)
{ d->selectionModel = model; }

void
Flow::animateCenterIndex(const QModelIndex &index)
{ d->scrollBar->setValue(index.row()); }

bool
Flow::isAnimating() const
{ return bool(d->timeLine->state() == QTimeLine::Running); }

float
Flow::y() const
{ return d->y; }

QList<Flow::Item *>
&Flow::items() const
{ return d->items; }

QColor
&Flow::bg() const
{ return d->bg; }
