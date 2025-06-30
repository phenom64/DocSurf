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

#include <QAction>
#include <QMenu>
#include <QProcess>
#include <QStyledItemDelegate>
#include <QTextLayout>
#include <QRect>
#include <qmath.h>
#include <QTextEdit>
#include <QMessageBox>
#include <QTextCursor>
#include <QTextFormat>
#include <QDesktopServices>
#include <QPainter>
#include <QApplication>
#include <QWheelEvent>
#include <QScrollBar>
#include <QSettings>
#include <QPropertyAnimation>
#include <QTransform>
#include <QDebug>
#include <QVector>

#include <KF5/KItemViews/KCategoryDrawer>
#include <KSharedConfig>
#include <KConfigGroup>

#include "viewcontainer.h"
#include "iconview.h"
#include "viewcontainer.h"
#include "mainwindow.h"
#include "viewanimator.h"
#include "objects.h"
#include "fsmodel.h"

using namespace KDFM;

//class ViewAdapter : public KAbstractViewAdapter
//{
//public:
//    ViewAdapter(QObject *parent) : KAbstractViewAdapter(parent) {}
//    virtual ~ViewAdapter() {}
//    virtual QAbstractItemModel *model() const { return static_cast<QAbstractItemView *>(parent())->model(); }
//    virtual QSize iconSize() const { return QSize(256, 256); }
//    virtual QPalette palette() const { return static_cast<QWidget *>(parent())->palette(); }
//    virtual QRect visibleArea() const
//    {
//        IconView *iconView = static_cast<IconView *>(parent());
//        return QRect(0, iconView->verticalOffset(), iconView->width(), iconView->height());
//    }
//    virtual QRect visualRect(const QModelIndex &index) const
//    {
//        IconView *iconView = static_cast<IconView *>(parent());
//        return iconView->visualRect(index);
//    }
//    virtual void connect(Signal signal, QObject *receiver, const char *slot)
//    {
//    }
//};

class IconDelegate : public FileItemDelegate
{
public:
    inline explicit IconDelegate(IconView *parent)
        : FileItemDelegate(parent)
        , m_iv(parent)
    {
    }
    void setEditorData(QWidget *editor, const QModelIndex &index) const
    {
        FileItemDelegate::setEditorData(editor, index);
        static_cast<QTextEdit *>(editor)->setAlignment(Qt::AlignCenter);
    }
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        FileItemDelegate::updateEditorGeometry(editor, option, index);
        const QRect geo = m_iv->visualRect(index);
        const int textHeight = editor->fontMetrics().height() * m_iv->textLines();
        QRect textRect = geo;
        textRect.setHeight(textHeight);
        textRect.moveBottom(geo.bottom());
        editor->setGeometry(textRect);
    }
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if (!index.isValid() || !option.rect.isValid())
            return;

        const QPen savedPen(painter->pen());
        const QBrush savedBrush(painter->brush());
        const bool selected(option.state & QStyle::State_Selected);
        const int step(selected ? ViewAnimator::Steps : ViewAnimator::hoverLevel(m_iv, index));
        if (step)
        {
            QStyleOptionViewItem copy(option);
            if (!selected)
                copy.state |= QStyle::State_MouseOver;
            QPixmap pix(option.rect.size());
            pix.fill(Qt::transparent);
            QPainter p(&pix);
            copy.rect = pix.rect();
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &copy, &p, m_iv);
            p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            p.fillRect(pix.rect(), QColor(0, 0, 0, ((255.0f/(float)ViewAnimator::Steps)*step)));
            p.end();
            painter->drawPixmap(option.rect, pix);
        }
        QRect textRect(option.rect), pixRect(option.rect);
        const int textHeight = option.fontMetrics.height() * m_iv->textLines();
        textRect.setHeight(textHeight);
        textRect.moveBottom(option.rect.bottom());
        pixRect.setBottom(textRect.top());
        const QPalette::ColorRole textRole = selected ? QPalette::HighlightedText : QPalette::Text;
        const bool enabled = option.state & QStyle::State_Enabled;
        QApplication::style()->drawItemText(painter, textRect, Qt::AlignTop|Qt::AlignHCenter, option.palette, enabled, text(option, index), textRole);

        QPixmap pixmap = option.icon.pixmap(m_iv->iconSize());
        if (pixmap.isNull())
            pixmap = index.data(Qt::DecorationRole).value<QIcon>().pixmap(m_iv->iconSize());

        QSize sz = pixmap.size();
        if (pixmap.size() != m_iv->iconSize()) //probably a thumb...
        {
            const qreal widthFactor = (qreal)pixRect.width()/(qreal)pixmap.width();
            const qreal heightFactor = (qreal)pixRect.height()/(qreal)pixmap.height();
            const qreal factor = qMax(widthFactor, heightFactor);
            sz = sz.scaled(sz.width()*factor, sz.height()*factor, Qt::KeepAspectRatio).boundedTo(pixRect.size());
            pixmap = option.icon.pixmap(sz);
            if (pixmap.isNull())
                pixmap = index.data(Qt::DecorationRole).value<QIcon>().pixmap(sz);
        }

        pixRect = QApplication::style()->itemPixmapRect(pixRect, Qt::AlignCenter, pixmap);
        QApplication::style()->drawItemPixmap(painter, pixRect, Qt::AlignCenter, pixmap);
        painter->setPen(savedPen);
        painter->setBrush(savedBrush);
    }
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return m_iv->gridSize();
    }
    inline void clearData() { m_textData.clear(); }
    inline void clear(const QModelIndex &index)
    {
        if (m_textData.contains(index))
            m_textData.remove(index);
    }
    bool isHitted(const QModelIndex &index, const QPoint &p, const QRect &r = QRect()) const
    {
        QRect theRect(QPoint(0,0), m_iv->iconSize());
        theRect.moveCenter(QRect(r.topLeft(), QSize(r.width(), m_iv->iconSize().height())).center());
        return theRect.contains(p);
    }
    inline void clearCache(const QModelIndex &index)
    {
//        qDebug() << "clear cache#" << index;
        m_textData.remove(index);
    }
protected:
    QString text(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if (!index.isValid())
            return QString();

        if (m_textData.contains(index))
            return m_textData.value(index);
        QFontMetrics fm(option.fontMetrics);

        QString spaces(index.data().toString().replace(".", QString(" ")));
        spaces = spaces.replace("_", QString(" "));
        QString theText;

        QTextLayout textLayout(spaces, option.font);
        int lineCount = -1;
        QTextOption opt;
        opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        textLayout.setTextOption(opt);

        const int w = option.rect.width();

        textLayout.beginLayout();
        while (++lineCount < m_iv->textLines())
        {
            QTextLine line = textLayout.createLine();
            if (!line.isValid())
                break;

            line.setLineWidth(w);
            QString actualText;
            actualText = index.data().toString().mid(line.textStart(), qMin(line.textLength(), index.data().toString().count()));
            if (line.lineNumber() == m_iv->textLines()-1)
                actualText = fm.elidedText(index.data().toString().mid(line.textStart(), index.data().toString().count()), Qt::ElideRight, w);

            //this should only happen if there
            //are actual dots or underscores...
            //so width should always be > oldw
            //we do this only cause QTextLayout
            //doesnt think that dots or underscores
            //are actual wordseparators
            if (fm.boundingRect(actualText).width() > w)
            {
                int width = 0;
                if (actualText.contains("."))
                    width += fm.boundingRect(".").width()*actualText.count(".");
                if (actualText.contains("_"))
                    width += fm.boundingRect("_").width()*actualText.count("_");

                int oldw = fm.boundingRect(" ").width()*actualText.count(" ");
                int diff = width - oldw;

                line.setLineWidth(w-diff);
                actualText = index.data().toString().mid(line.textStart(), qMin(line.textLength(), index.data().toString().count()));
                if (line.lineNumber() == m_iv->textLines()-1)
                    actualText = fm.elidedText(index.data().toString().mid(line.textStart(), index.data().toString().count()), Qt::ElideRight, w);
            }
            theText.append(QString("%1\n").arg(actualText));
        }
        textLayout.endLayout();
        m_textData.insert(index, theText);
        return theText;
    }
    static inline int textFlags() { return Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap; }

private:
    IconView *m_iv;
    mutable QHash<QModelIndex, QString> m_textData;
};

class IconView::Private
{
public:
    IconView *q;
    QPoint pressPos;
    QModelIndex pressedIndex;
    int textLines;
    Private(IconView *view) : q(view) {}
};

class CategoryDrawer : public KCategoryDrawer
{
public:
    explicit CategoryDrawer(KCategorizedView *view) : KCategoryDrawer(view)
    {

    }
    void drawCategory(const QModelIndex &index,
                              int sortRole,
                              const QStyleOption &option,
                              QPainter *painter) const
    {
        // Keep this in sync with Kirigami.ListSectionHeader
        painter->setRenderHint(QPainter::Antialiasing);

        const QString category = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString();
        QFont font(QApplication::font());
        // Match Heading with level 3
        font.setPointSizeF(font.pointSize() * 1.20);
        const QFontMetrics fontMetrics = QFontMetrics(font);

        QColor backgroundColor = option.palette.base().color();

        //BEGIN: background
//        {
            QRect backgroundRect(option.rect);
            backgroundRect.setHeight(categoryHeight(index, option));
            painter->fillRect(backgroundRect, QColor(0,0,0,15));
            QRect top = backgroundRect;
            top.setBottom(top.top());
            if (index.row() != 0)
                painter->fillRect(top, QColor(0,0,0,63));
            top.moveTop(backgroundRect.bottom());
            painter->fillRect(top, QColor(0,0,0,63));
//            painter->save();
//            painter->setBrush(backgroundColor);
//            painter->setPen(Qt::NoPen);
//            painter->drawRect(backgroundRect);
//            painter->restore();
//        }
        //END: background

        //BEGIN: text
//        {
            //  Kirgami.Units.{small/large}Spacing respectively
            painter->save();
            font.setLetterSpacing(QFont::AbsoluteSpacing, 2);
            painter->setFont(font);
            painter->setPen(option.palette.text().color());
            painter->drawText(backgroundRect, Qt::AlignCenter, category);
            painter->restore();
//            constexpr int topPadding = 4;
//            constexpr int sidePadding = 8;
//            QRect textRect(option.rect);
//            textRect.setTop(textRect.top() + topPadding);
//            textRect.setLeft(textRect.left() + sidePadding);
//            textRect.setHeight(fontMetrics.height());
//            textRect.setRight(textRect.right() - sidePadding);

//            painter->save();
//            painter->setFont(font);
//            QColor penColor(option.palette.text().color());
//            painter->setPen(penColor);
//            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, category);
//            painter->restore();
//        }
        //END: text
        //        KCategoryDrawer::drawCategory(index, sortRole, option, painter);
    }
};

IconView::IconView(QWidget *parent)
    : KCategorizedView(parent)
    , Configurable()
    , d(new Private(this))
{
//    connect(static_cast<MainWindow *>(window()), &MainWindow::reconfigured, this, &IconView::reconfigure);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::DragDrop);
    setMovement(QListView::Snap);
    setViewMode(QListView::IconMode);
    connect(this, &QAbstractItemView::iconSizeChanged, this, &IconView::updateLayout);
    itemDelegate()->deleteLater();
    setItemDelegate(new IconDelegate(this));
    setCollapsibleBlocks(true);
//    setItemDelegate(new FileItemDelegate(this));
    CategoryDrawer *drawer = new CategoryDrawer(this);
    setCategoryDrawer(drawer);

    reconfigure();
}

IconView::~IconView()
{
    delete d;
}

int
IconView::textLines() const
{ return d->textLines; }

void
IconView::reconfigure()
{
    KConfigGroup config = KSharedConfig::openConfig("kdfm.conf")->group("Views");
    d->textLines = config.readEntry("IconTextLines", 3);
    updateLayout();
}

void
IconView::setModel(QAbstractItemModel *model)
{
    KCategorizedView::setModel(model);
    ViewAnimator::manage(this);
    ScrollAnimator::manage(this);
    updateLayout();
    connect(model, &QAbstractItemModel::layoutChanged, this, &IconView::updateLayout);
    connect(model, &QAbstractItemModel::rowsInserted, this, &IconView::updateLayout);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &IconView::updateLayout);
    connect(model, &QAbstractItemModel::rowsMoved, this, &IconView::updateLayout);
    connect(model, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>())
    {
        if (roles.isEmpty()) //item renamed
            for (int i = topLeft.row(); i <= bottomRight.row(); ++i)
                static_cast<IconDelegate *>(itemDelegate())->clearCache(static_cast<QAbstractItemModel *>(sender())->index(i, 0, topLeft.parent()));
    });
}

void
IconView::contextMenuEvent(QContextMenuEvent *e)
{
    e->accept();
    emit requestRightClickMenu(e->globalPos());
}

void
IconView::mouseDoubleClickEvent(QMouseEvent *event)
{
    KCategorizedView::mouseDoubleClickEvent(event);
    const QModelIndex &index = indexAt(event->pos());
    if (index.isValid()
            && true/*Store::config.views.singleClick*/
            && !event->modifiers()
            && event->button() == Qt::LeftButton
            && state() == NoState)
        emit opened(index);
}

void
IconView::mouseReleaseEvent(QMouseEvent *e)
{
//    setDragEnabled(true);
    const QModelIndex &index = indexAt(e->pos());

    if (!index.isValid())
    {
        KCategorizedView::mouseReleaseEvent(e);
        return;
    }

    const int em = 5; //errormargin.. get this from somewhere... after this we start a dnd op
    const QRect errorRect(d->pressPos-QPoint(em, em), d->pressPos+QPoint(em, em));

    if (false/*Store::config.views.singleClick*/
            && !e->modifiers()
            && e->button() == Qt::LeftButton
            && d->pressedIndex == index
            && errorRect.contains(e->pos())
            && !index.column())
    {
        emit opened(index);
        e->accept();
//        return;
    }

    if (e->button() == Qt::MiddleButton
            && errorRect.contains(e->pos())
            && !e->modifiers())
    {
        e->accept();
        emit newTabRequest(index);
//        return;
    }

    KCategorizedView::mouseReleaseEvent(e);
}

void
IconView::mousePressEvent(QMouseEvent *event)
{
//    if (event->modifiers() & Qt::MetaModifier || indexAt(event->pos()).column())
//        setDragEnabled(false);
    d->pressedIndex = indexAt(event->pos());
    d->pressPos = event->pos();
    KCategorizedView::mousePressEvent(event);
}

void
IconView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::CTRL)
    {
        MainWindow *mw = static_cast<MainWindow *>(window());
        if (QSlider *s = mw->iconSizeSlider())
            s->setValue(s->value()+(e->delta()>0?1:-1));
    }
    else
        KCategorizedView::wheelEvent(e);
    e->accept();
}

QModelIndex
IconView::indexAt(const QPoint &point) const
{
    const QModelIndex &index = KCategorizedView::indexAt(point);
//    return index;
    IconDelegate *delegate = static_cast<IconDelegate *>(itemDelegate());
    if (delegate->isHitted(index, point, visualRect(index)))
        return index;
    return QModelIndex();
}

void
IconView::reset()
{
    KCategorizedView::reset();
    QMetaObject::invokeMethod(this, "updateLayout", Qt::QueuedConnection);
}

void
IconView::resizeEvent(QResizeEvent *e)
{
    KCategorizedView::resizeEvent(e);
    if (e->size().width() != e->oldSize().width())
        updateLayout();
}

void
IconView::updateLayout()
{
    if (!model())
        return;
    const int scrollExt = style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, verticalScrollBar());
    const int frameExt = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);
    int contentsWidth = /*viewport()->*/width()-(scrollExt+frameExt*2+1);
    int horItemCount = contentsWidth / (iconSize().width() + 32/*Store::config.views.iconView.textWidth*/ * 2);
//    const int rowCount = model()->rowCount(rootIndex());
//    if (rowCount < horItemCount && rowCount > 1)
//        horItemCount = rowCount;
    if (contentsWidth && horItemCount)
    {
        const int gridWidth = contentsWidth/horItemCount;
        const int gridHeight = iconSize().height() + (d->textLines/*Store::config.views.iconView.lineCount*/ * fontMetrics().height());
        const QSize gridSize(gridWidth, gridHeight);
        KCategorizedView::setGridSizeOwn(gridSize);
    }
    static_cast<IconDelegate *>(itemDelegate())->clearData();
}
