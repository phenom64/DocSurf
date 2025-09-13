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

#include <QTextEdit>
#include <QMessageBox>
#include <QScrollBar>
#include <QTimer>
#include <QPainter>
#include <QKeyEvent>
#include <QDebug>
#include <QAbstractItemView>
#include <QItemSelectionModel>

#include "objects.h"
#include "application.h"
#include "fsmodel.h"
#include "gfx/fx.h"

using namespace DocSurf;

QPixmap *FileItemDelegate::s_shadowData(0);

FileItemDelegate::FileItemDelegate(QObject *parent)
    : KFileItemDelegate(parent)
{
    if (!s_shadowData)
        genShadowData();
}

QWidget
*FileItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return KFileItemDelegate::createEditor(parent, option, index);
}

void
FileItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    KFileItemDelegate::setEditorData(editor, index);
}

void
FileItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    KFileItemDelegate::setModelData(editor, model, index);
}

bool
FileItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    return KFileItemDelegate::eventFilter(object, event);
}

void
FileItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    KFileItemDelegate::updateEditorGeometry(editor, option, index);
}

QPixmap
FileItemDelegate::shadowPix()
{
    const int size((shadowSize()*2)+1);
    QImage img(size, size, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter pt(&img);
    QRectF rect(img.rect());
    pt.setRenderHint(QPainter::Antialiasing);
    pt.setBrush(Qt::NoBrush);
    pt.setPen(Qt::NoPen);
    pt.fillRect(rect.adjusted(2.75f, 3.0f, -2.75f, -2.5f), QColor(0, 0, 0, 170));
    pt.end();
    FX::expblur(img, 1);
    pt.begin(&img);
    pt.fillRect(rect.adjusted(3, 3, -3, -3), QColor(0, 0, 0, 85));
    pt.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    pt.fillRect(rect.adjusted(4, 4, -4, -4), Qt::black);
//    pt.setCompositionMode(QPainter::CompositionMode_SourceOver);
//    pt.setPen(QColor(255, 255, 255, 63));
//    const QRect r = img.rect().adjusted(4, 4, -4, -4);
//    pt.drawLine(r.topLeft(), r.topRight());
    pt.end();
    QPixmap pix = QPixmap::fromImage(img);
    return pix;
}

void
FileItemDelegate::genShadowData()
{
    const QPixmap shadow = shadowPix();
    const int w(shadow.width()), h(shadow.height());
    s_shadowData = new QPixmap[9]();
    s_shadowData[TopLeft] =     shadow.copy(0, 0, shadowSize(), shadowSize());
    s_shadowData[Top] =         shadow.copy(shadowSize(), 0, 1, shadowSize());
    s_shadowData[TopRight] =    shadow.copy(w-shadowSize(), 0, shadowSize(), shadowSize());
    s_shadowData[Left] =        shadow.copy(0, shadowSize(), shadowSize(), 2);
    s_shadowData[Center] =      QPixmap(); //no center...
    s_shadowData[Right] =       shadow.copy(w-shadowSize(), shadowSize(), shadowSize(), 1);
    s_shadowData[BottomLeft] =  shadow.copy(0, h-shadowSize(), shadowSize(), shadowSize());
    s_shadowData[Bottom] =      shadow.copy(shadowSize(), h-shadowSize(), 1, shadowSize());
    s_shadowData[BottomRight] = shadow.copy(w-shadowSize(), h-shadowSize(), shadowSize(), shadowSize());
}

void
FileItemDelegate::drawShadow(QRect rect, QPainter *painter)
{
    const int x = rect.x(), y = rect.y(), w = rect.width(), h = rect.height(), r = x+w, b = y+h;
    painter->drawPixmap(QRect(x, y, shadowSize(), shadowSize()), s_shadowData[TopLeft]);
    painter->drawTiledPixmap(QRect(x+shadowSize(), y, w-(shadowSize()*2), shadowSize()), s_shadowData[Top]);
    painter->drawPixmap(QRect(r-shadowSize(), y, shadowSize(), shadowSize()), s_shadowData[TopRight]);
    painter->drawTiledPixmap(QRect(x, y+shadowSize(), shadowSize(), h-(shadowSize()*2)), s_shadowData[Left]);
    painter->drawTiledPixmap(QRect(r-shadowSize(), y+shadowSize(), shadowSize(), h-(shadowSize()*2)), s_shadowData[Right]);
    painter->drawPixmap(QRect(x, b-shadowSize(), shadowSize(), shadowSize()), s_shadowData[BottomLeft]);
    painter->drawTiledPixmap(QRect(x+shadowSize(), b-shadowSize(), w-(shadowSize()*2), shadowSize()), s_shadowData[Bottom]);
    painter->drawPixmap(QRect(r-shadowSize(), b-shadowSize(), shadowSize(), shadowSize()), s_shadowData[BottomRight]);
}

//-------------------------------------------------------------------------------------------

ScrollAnimator::ScrollAnimator(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_up(false)
    , m_delta(0)
    , m_step(0)
{
    if (!parent)
    {
        deleteLater();
        return;
    }
    connect(m_timer, &QTimer::timeout, this, &ScrollAnimator::updateScrollValue);
    static_cast<QAbstractScrollArea *>(parent)->viewport()->installEventFilter(this);
}

void
ScrollAnimator::manage(QAbstractScrollArea *area)
{
    if (!area || area->findChild<ScrollAnimator *>())
        return;
    new ScrollAnimator(area);
}

#define MAXDELTA 80

void
ScrollAnimator::updateScrollValue()
{

    if (m_delta > 0)
    {
        QScrollBar *bar = static_cast<QAbstractScrollArea *>(parent())->verticalScrollBar();
        if ((m_up && bar->value() == bar->minimum())
               || (!m_up && bar->value() == bar->maximum()))
        {
            m_delta = 0;
            m_timer->stop();
            return;
        }
        bar->setValue(bar->value() + (m_up?-m_delta:m_delta));
        m_delta -= m_step;
    }
    else
    {
        m_delta = 0;
        m_timer->stop();
    }
}

bool
ScrollAnimator::processWheelEvent(QWheelEvent *e)
{
    if (e->orientation() == Qt::Horizontal || e->modifiers())
        return false;
    const bool wasUp(m_up);
    m_up = e->angleDelta().y() > 0;
    if (m_up != wasUp)
        m_delta = 0;

    m_delta+=10;
    m_step=m_delta/10;
    if (!m_timer->isActive())
        m_timer->start(20);
    return true;
}

bool
ScrollAnimator::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Wheel)
        return processWheelEvent(static_cast<QWheelEvent *>(e));
    return false;
}
