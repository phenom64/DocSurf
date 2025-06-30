#ifndef COLOR_H
#define COLOR_H

#include <QColor>

class Q_DECL_EXPORT Color
{
public:
    static QColor mid(const QColor &c1, const QColor c2, int i1 = 1, int i2 = 1);
    static bool contrast(const QColor &c1, const QColor &c2);
    static void ensureContrast(QColor &c1, QColor &c2);
    static void setValue(const int value, QColor &c);
    static int lum(const QColor &c);
    static void shiftHue(QColor &c, int amount);
    static QColor complementary(QColor c);
};

#endif // COLOR_H
