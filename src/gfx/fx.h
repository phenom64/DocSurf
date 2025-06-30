#ifndef FX_H
#define FX_H

#include <QtGlobal>
#include <QSize>
#include <qnamespace.h>
class QImage;
class QRect;
class QPixmap;
class QBrush;
class QColor;

namespace FX
{
    void expblur(QImage &img, int radius, Qt::Orientations o = Qt::Horizontal|Qt::Vertical);
    QPixmap mid(const QPixmap &p1, const QBrush &b, const int a1 = 1, const int a2 = 1, const QSize &sz = QSize());
    QPixmap mid(const QPixmap &p1, const QPixmap &p2, const int a1 = 1, const int a2 = 1, const QSize &sz = QSize());
    void colortoalpha(float *a1, float *a2, float *a3, float *a4, float c1, float c2, float c3);
    QPixmap sunkenized(const QRect &r, const QPixmap &source, const bool isDark = false, const int shadowOpacity = 127);
    int stretch(const int v, const float n = 1.5f);
    int pushed(const float v, const float inlo, const float inup, const float outlo = 0.0f, const float outup = 255.0f);
    QImage stretched(QImage img);
    QImage stretched(QImage img, const QColor &c);
    void autoStretch(QImage &img);
    void colorizePixmap(QPixmap &pix, const QBrush &b);
    QPixmap colorized(QPixmap pix, const QBrush &b);
}

#endif // FX_H
