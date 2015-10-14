/***************************************************************************
                          imgsource.h  -  description
                             -------------------
    begin                : 14 April 2007
    copyright            : (C) 2007 by Adam Davison
    email                : adamdavison@gmail.com
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
#include <QImage>
#include <QColor>
#include <QString>
#include <QRgb>
#include <QtDebug>

class ImgSource {
  public:
    virtual ~ImgSource();
    virtual QImage* getImage(QString img) const = 0;
    virtual QColor getCorrectColor(QColor c) const;
    virtual void correctImageColors(QImage* p) const;
};

class ImgProcessor : public ImgSource {

  public:
    virtual ~ImgProcessor();
    ImgProcessor(ImgSource* parent);
    virtual QColor doColorCorrection(QColor c)const = 0;
    virtual QColor getCorrectColor(QColor c) const;
    virtual void correctImageColors(QImage* p) const;
  protected:
    ImgSource* m_parent = nullptr;
};
class ImgColorProcessor : public ImgProcessor {
public:
    virtual ~ImgColorProcessor();
    ImgColorProcessor(ImgSource* parent);
    virtual QImage* getImage(QString img) const;
    virtual void correctImageColors(QImage* i) const;
};
