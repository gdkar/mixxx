/***************************************************************************
                          wpixmapstore.h  -  description
                             -------------------
    begin                : Mon Jun 28 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

_Pragma("once")
#include <QPixmap>
#include <QHash>
#include <QSharedPointer>
#include <QSvgRenderer>
#include <QImage>
#include <QScopedPointer>
#include <QPainter>
#include <QRectF>
#include <QString>

#include "skin/imgsource.h"
#include "skin/pixmapsource.h"

// Wrapper around QImage and QSvgRenderer to support rendering SVG images in
// high fidelity.
class Paintable {
  public:
    enum DrawMode {
        // Draw the image in its native dimensions with no stretching or tiling.
        FIXED,
        // Stretch the image.
        STRETCH,
        // Stretch the image maintaining its aspect ratio.
        STRETCH_ASPECT,
        // Tile the image.
        TILE
    };
    // Takes ownership of QImage.
    Paintable(QImage pImage, DrawMode mode);
    Paintable(QString fileName, DrawMode mode);
    Paintable(const PixmapSource& source, DrawMode mode);
    QSize size() const;
    int width() const;
    int height() const;
    QRectF rect() const;
    DrawMode drawMode() const;
    void draw(int x, int y, QPainter* pPainter);
    void draw(QPointF point, QPainter* pPainter,QRectF sourceRect);
    void draw(QRectF targetRect, QPainter* pPainter);
    void draw(QRectF targetRect, QPainter* pPainter,QRectF sourceRect);
    void drawCentered(QRectF targetRect, QPainter* pPainter,QRectF sourceRect);
    bool isNull() const;
    static DrawMode DrawModeFromString(QString str);
    static QString DrawModeToString(DrawMode mode);
  private:
    void drawInternal(QRectF targetRect, QPainter* pPainter,QRectF sourceRect);
    QPixmap                 m_pPixmap;
    QSvgRenderer             m_pSvg;
    DrawMode m_draw_mode;
};
typedef QSharedPointer<Paintable> PaintablePointer;
typedef QWeakPointer<Paintable> WeakPaintablePointer;
class WPixmapStore {
  public:
    static PaintablePointer getPaintable(PixmapSource source,Paintable::DrawMode mode);
    static QPixmap getPixmapNoCache(QString fileName);
    static void setLoader(QSharedPointer<ImgSource> ld);
  private:
    static QHash<QString, WeakPaintablePointer> m_paintableCache;
    static QSharedPointer<ImgSource> m_loader;
};

