#include "fx.h"
#include <math.h>
#include "color.h"
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QSize>
#include <QBrush>
#include <QDebug>

/*
// Exponential blur, Jani Huhtanen, 2006 ==========================
*  expblur(QImage &img, int radius)
*
*  In-place blur of image 'img' with kernel of approximate radius 'radius'.
*  Blurs with two sided exponential impulse response.
*
*  aprec = precision of alpha parameter in fixed-point format 0.aprec
*  zprec = precision of state parameters zR,zG,zB and zA in fp format 8.zprec
*/

template<int aprec, int zprec>
static inline void blurinner(unsigned char *bptr, int &zR, int &zG, int &zB, int &zA, int alpha)
{
    int R,G,B,A;
    R = *bptr;
    G = *(bptr+1);
    B = *(bptr+2);
    A = *(bptr+3);

    zR += (alpha * ((R<<zprec)-zR))>>aprec;
    zG += (alpha * ((G<<zprec)-zG))>>aprec;
    zB += (alpha * ((B<<zprec)-zB))>>aprec;
    zA += (alpha * ((A<<zprec)-zA))>>aprec;

    *bptr =     zR>>zprec;
    *(bptr+1) = zG>>zprec;
    *(bptr+2) = zB>>zprec;
    *(bptr+3) = zA>>zprec;
}

template<int aprec,int zprec>
static inline void blurrow( QImage & im, int line, int alpha)
{
    int zR,zG,zB,zA;

    QRgb *ptr = (QRgb *)im.scanLine(line);

    zR = *((unsigned char *)ptr    )<<zprec;
    zG = *((unsigned char *)ptr + 1)<<zprec;
    zB = *((unsigned char *)ptr + 2)<<zprec;
    zA = *((unsigned char *)ptr + 3)<<zprec;

    for(int index=1; index<im.width(); index++)
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);

    for(int index=im.width()-2; index>=0; index--)
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);
}

template<int aprec, int zprec>
static inline void blurcol( QImage & im, int col, int alpha)
{
    int zR,zG,zB,zA;

    QRgb *ptr = (QRgb *)im.bits();
    ptr+=col;

    zR = *((unsigned char *)ptr    )<<zprec;
    zG = *((unsigned char *)ptr + 1)<<zprec;
    zB = *((unsigned char *)ptr + 2)<<zprec;
    zA = *((unsigned char *)ptr + 3)<<zprec;

    for(int index=im.width(); index<(im.height()-1)*im.width(); index+=im.width())
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);

    for(int index=(im.height()-2)*im.width(); index>=0; index-=im.width())
        blurinner<aprec,zprec>((unsigned char *)&ptr[index],zR,zG,zB,zA,alpha);
}

void
FX::expblur(QImage &img, int radius, Qt::Orientations o)
{
    if(radius<1)
        return;

    static const int aprec = 16; static const int zprec = 7;

    // Calculate the alpha such that 90% of the kernel is within the radius. (Kernel extends to infinity)
    int alpha = (int)((1<<aprec)*(1.0f-expf(-2.3f/(radius+1.f))));

    if (o & Qt::Horizontal) {
        for(int row=0;row<img.height();row++)
            blurrow<aprec,zprec>(img,row,alpha);
    }

    if (o & Qt::Vertical) {
        for(int col=0;col<img.width();col++)
            blurcol<aprec,zprec>(img,col,alpha);
    }
}

QPixmap
FX::mid(const QPixmap &p1, const QPixmap &p2, const int a1, const int a2, const QSize &sz)
{
    int w(qMin(p1.width(), p2.width()));
    int h(qMin(p1.height(), p2.height()));
    if (sz.isValid())
    {
        w = sz.width();
        h = sz.height();
    }
    QImage i1 = p1.copy(0, 0, w, h).toImage().convertToFormat(QImage::Format_ARGB32);
    QImage i2 = p2.copy(0, 0, w, h).toImage().convertToFormat(QImage::Format_ARGB32);
    const int size(w*h);
    QImage i3(w, h, QImage::Format_ARGB32); //the resulting image
    i3.fill(Qt::transparent);
    QRgb *rgb1 = reinterpret_cast<QRgb *>(i1.bits());
    QRgb *rgb2 = reinterpret_cast<QRgb *>(i2.bits());
    QRgb *rgb3 = reinterpret_cast<QRgb *>(i3.bits());

    for (int i = 0; i < size; ++i)
        rgb3[i] = Color::mid(QColor::fromRgba(rgb1[i]), QColor::fromRgba(rgb2[i]), a1, a2).rgba();
    return QPixmap::fromImage(i3);
}

QPixmap
FX::mid(const QPixmap &p1, const QBrush &b, const int a1, const int a2, const QSize &sz)
{
    QPixmap pix1;
    if (sz.width() > p1.width() || sz.height() > p1.height())
    {
        pix1 = QPixmap(sz);
        pix1.fill(Qt::transparent);
        QPainter p(&pix1);
        p.drawTiledPixmap(pix1.rect(), p1);
        p.end();
    }
    QPixmap p2(sz.isValid()?sz:p1.size());
    p2.fill(Qt::transparent);
    QPainter p(&p2);
    p.fillRect(p2.rect(), b);
    p.end();
    return mid(pix1.isNull()?p1:pix1, p2, a1, a2, sz);
}

//colortoalpha directly stolen from gimp

void
FX::colortoalpha (float *a1,
          float *a2,
          float *a3,
          float *a4,
          float c1,
          float c2,
          float c3)
{
  float alpha1, alpha2, alpha3, alpha4;

  alpha4 = *a4;

  if ( *a1 > c1 )
    alpha1 = (*a1 - c1)/(255.0-c1);
  else if ( *a1 < c1 )
    alpha1 = (c1 - *a1)/(c1);
  else alpha1 = 0.0;

  if ( *a2 > c2 )
    alpha2 = (*a2 - c2)/(255.0-c2);
  else if ( *a2 < c2 )
    alpha2 = (c2 - *a2)/(c2);
  else alpha2 = 0.0;

  if ( *a3 > c3 )
    alpha3 = (*a3 - c3)/(255.0-c3);
  else if ( *a3 < c3 )
    alpha3 = (c3 - *a3)/(c3);
  else alpha3 = 0.0;

  if ( alpha1 > alpha2 )
    if ( alpha1 > alpha3 )
      {
    *a4 = alpha1;
      }
    else
      {
    *a4 = alpha3;
      }
  else
    if ( alpha2 > alpha3 )
      {
    *a4 = alpha2;
      }
    else
      {
    *a4 = alpha3;
      }

  *a4 *= 255.0;

  if ( *a4 < 1.0 )
    return;
  *a1 = 255.0 * (*a1-c1)/ *a4 + c1;
  *a2 = 255.0 * (*a2-c2)/ *a4 + c2;
  *a3 = 255.0 * (*a3-c3)/ *a4 + c3;

  *a4 *= alpha4/255.0;
}

QPixmap
FX::sunkenized(const QRect &r, const QPixmap &source, const bool isDark, const int shadowOpacity)
{
    int m(4);
    QImage img(r.size()+QSize(m, m), QImage::Format_ARGB32);
    m/=2;
    img.fill(Qt::transparent);
    QPainter p(&img);
    int rgb(isDark?255:0);
    int alpha(/*qMin(*/shadowOpacity/*, 2*dConf.shadows.opacity)*/);
    p.fillRect(img.rect(), QColor(rgb, rgb, rgb, alpha));
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    p.drawTiledPixmap(r.translated(m, m), source);
    p.end();
    QImage blur(img);
    FX::expblur(blur, m);
    QPixmap highlight(r.size());
    highlight.fill(Qt::transparent);
    p.begin(&highlight);
    rgb = isDark?0:255;
    p.fillRect(highlight.rect(), QColor(rgb, rgb, rgb, alpha));
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawTiledPixmap(highlight.rect(), source);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    p.drawTiledPixmap(highlight.rect().translated(0, -1), source);
    p.end();

    QPixmap pix(r.size());
    pix.fill(Qt::transparent);
    p.begin(&pix);
    p.drawTiledPixmap(pix.rect(), source);
    if (!isDark)
    {
        p.setCompositionMode(QPainter::CompositionMode_Overlay);
        p.drawTiledPixmap(pix.rect().translated(0, 1), QPixmap::fromImage(blur), QPoint(m, m));
    }
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawTiledPixmap(pix.rect(), source);
    p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    p.drawTiledPixmap(pix.rect().translated(0, 1), highlight);
    p.end();
    return pix;
}

int
FX::stretch(const int v, const float n)
{
    static bool isInit(false);
    static float table[256];
    if (!isInit)
    {
        for (int i = 127; i < 256; ++i)
            table[i] = (pow(((float)i/127.5f-1.0f), (1.0f/n))+1.0f)*127.5f;
        for (int i = 0; i < 128; ++i)
            table[i] = 255-table[255-i];
        isInit = true;
    }
    return qRound(table[v]);
}

/**
    http://spatial-analyst.net/ILWIS/htm/ilwisapp/stretch_algorithm.htm
    OUTVAL
    (INVAL - INLO) * ((OUTUP-OUTLO)/(INUP-INLO)) + OUTLO
    where:
    OUTVAL
    Value of pixel in output map
    INVAL
    Value of pixel in input map
    INLO
    Lower value of 'stretch from' range
    INUP
    Upper value of 'stretch from' range
    OUTLO
    Lower value of 'stretch to' range
    OUTUP
    Upper value of 'stretch to' range
 */

int
FX::pushed(const float v, const float inlo, const float inup, const float outlo, const float outup)
{
    return qRound((v - inlo) * ((outup-outlo)/(inup-inlo)) + outlo);
}

QImage
FX::stretched(QImage img)
{
    QImage image = img.convertToFormat(QImage::Format_ARGB32);
    int size = img.width() * img.height();
    QRgb *pixels = reinterpret_cast<QRgb *>(img.bits());
    for (int i = 0; i < size; ++i)
    {
        const int rgb(stretch(qGray(pixels[i])));
        pixels[i] = qRgb(rgb, rgb, rgb);
    }
    return image;
}

QImage
FX::stretched(QImage img, const QColor &c)
{
    img = img.convertToFormat(QImage::Format_ARGB32);
    int size = img.width() * img.height();
    QRgb *pixels[2] = { 0, 0 };
    pixels[0] = reinterpret_cast<QRgb *>(img.bits());
    int r, g, b;
    c.getRgb(&r, &g, &b); //foregroundcolor

#define ENSUREALPHA if (!qAlpha(pixels[0][i])) continue
    QList<int> hues;
    for (int i = 0; i < size; ++i)
    {
        ENSUREALPHA;
        const int hue = QColor::fromRgba(pixels[0][i]).hue();
        if (!hues.contains(hue))
            hues << hue;
    }

    const bool useAlpha(hues.count() == 1);
    if (useAlpha)
    {
        int l(0), h(0);
        for (int i = 0; i < size; ++i)
        {
            ENSUREALPHA;
            const QColor c = QColor::fromRgba(pixels[0][i]);
            if (Color::lum(c) < 128)
                ++l;
            else
                ++h;
        }
        const bool dark(l*2>h);
        /**
          * Alpha gets special treatment...
          * we simply steal the colortoalpha
          * function from gimp, color white to
          * alpha, as in remove all white from
          * the image (in case of monochrome icons
          * add some sorta light bevel). Then simply
          * push all remaining alpha values so the
          * highest one is 255.
          */
        float alpha[size], lowAlpha(255), highAlpha(0);
        for (int i = 0; i < size; ++i)
        {
            ENSUREALPHA;
            const QRgb rgb(pixels[0][i]);
            float val(dark?255.0f:0.0f);
            float red(qRed(rgb)), green(qGreen(rgb)), blue(qBlue(rgb)), a(qAlpha(rgb));
            FX::colortoalpha(&red, &green, &blue, &a, val, val, val);
            if (a < lowAlpha)
                lowAlpha = a;
            if (a > highAlpha)
                highAlpha = a;
            alpha[i] = a;
        }
        float add(255.0f/highAlpha);
        for (int i = 0; i < size; ++i)
        {
            ENSUREALPHA;
            pixels[0][i] = qRgba(r, g, b, stretch(qMin<int>(255, alpha[i]*add), 2.0f));
        }
        return img;
    }

    const int br(img.width()/4);
    QImage bg(img.size() + QSize(br*2, br*2), QImage::Format_ARGB32); //add some padding avoid 'edge folding'
    bg.fill(Qt::transparent);

    QPainter bp(&bg);
    bp.drawPixmap(br, br, QPixmap::fromImage(img), 0, 0, img.width(), img.height());
    bp.end();
    FX::expblur(bg, br);
    bg = bg.copy(bg.rect().adjusted(br, br, -br, -br)); //remove padding so we can easily access relevant pixels with [i]

    enum Rgba { Red = 0, Green, Blue, Alpha };
    enum ImageType { Fg = 0, Bg }; //fg is the actual image, bg is the blurred one we use as reference for the most contrasting channel

    pixels[1] = reinterpret_cast<QRgb *>(bg.bits());
    double rgbCount[3] = { 0.0d, 0.0d, 0.0d };
    int (*rgba[4])(QRgb rgb) = { qRed, qGreen, qBlue, qAlpha };
    int cVal[2][4] = {{ 0, 0, 0, 0 }, {0, 0, 0, 0}};

    for (int i = 0; i < size; ++i) //pass 1, determine most contrasting channel, usually RED
    for (int it = 0; it < 2; ++it)
    for (int d = 0; d < 4; ++d)
    {
        ENSUREALPHA;
        cVal[it][d] = (*rgba[d])(pixels[it][i]);
        if (it && d < 3)
            rgbCount[d] += (cVal[Fg][Alpha]+cVal[Bg][Alpha]) * qAbs(cVal[Fg][d]-cVal[Bg][d]) / (2.0f * cVal[Bg][d]);
    }
    double count = 0;
    int channel = 0;
    for (int i = 0; i < 3; ++i)
        if (rgbCount[i] > count)
        {
            channel = i;
            count = rgbCount[i];
        }
    float values[size];
    float inLo = 255.0f, inHi = 0.0f;
    qulonglong loCount(0), hiCount(0);
    for ( int i = 0; i < size; ++i ) //pass 2, store darkest/lightest pixels
    {
        ENSUREALPHA;
        const float px((*rgba[channel])(pixels[Fg][i]));
        values[i] = px;
        if ( px > inHi )
            inHi = px;
        if ( px < inLo )
            inLo = px;
        const int a(qAlpha(pixels[Fg][i]));
        if (px < 128)
            loCount += a*(255-px);
        else
            hiCount += a*px;
    }
    const bool isDark(loCount>hiCount);
    for ( int i = 0; i < size; ++i )
    {
        ENSUREALPHA;
        int a(qBound<int>(0, qRound((values[i] - inLo) * (255.0f / (inHi - inLo))), 255));
        if (isDark)
            a = qMax(0, qAlpha(pixels[Fg][i])-a);
        pixels[Fg][i] = qRgba(r, g, b, stretch(a));
    }
#undef ENSUREALPHA
    return img;
}

//QPixmap
//FX::monochromized(const QPixmap &source, const QColor &color, const Effect effect, bool isDark)
//{
//    const QPixmap &result = QPixmap::fromImage(stretched(source.toImage(), color));
//    if (effect == Inset && dConf.shadows.onTextOpacity)
//        return sunkenized(result.rect(), result, isDark);
//    return result;
//}

void
FX::colorizePixmap(QPixmap &pix, const QBrush &b)
{
    QPixmap copy(pix);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.fillRect(pix.rect(), b);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.drawPixmap(pix.rect(), copy);
    p.end();
}

QPixmap
FX::colorized(QPixmap pix, const QBrush &b)
{
    colorizePixmap(pix, b);
    return pix;
}

void
FX::autoStretch(QImage &img)
{
    if (img.format() != QImage::Format_ARGB32)
        img = img.convertToFormat(QImage::Format_ARGB32);
    const int size = img.width() * img.height();
    QRgb *px = reinterpret_cast<QRgb *>(img.bits());
    int inLo(255);
    int inUp(0);
    for (int i = 0; i < size; ++i) //determine lowest and highest pixels
    {
        const QColor pxc = QColor::fromRgba(px[i]);
        const int lum = Color::lum(pxc);
        if (lum > inUp)
            inUp = lum;
        if (lum < inLo)
            inLo = lum;
        px[i] = qRgba(lum, lum, lum, qAlpha(px[i]));
    }
    for (int i = 0; i < size; ++i)
    {
        const int v = pushed(qGray(px[i]), inLo, inUp);
        px[i] = qRgba(v,v,v,qAlpha(px[i]));
    }
}

