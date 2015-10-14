/***************************************************************************
                          imgcolor.h  -  description
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
#include "imgsource.h"

class ImgAdd : public ImgColorProcessor {
public:
    ImgAdd(ImgSource* parent, int amt);
    virtual ~ImgAdd();
    virtual QColor doColorCorrection(QColor c) const;
private:
    int m_amt = 0;
};
class ImgMax : public ImgColorProcessor {
public:
    ImgMax(ImgSource* parent, int amt);
    virtual ~ImgMax();
    virtual QColor doColorCorrection(QColor c) const;
private:
    int m_amt = 0;
};
class ImgScaleWhite : public ImgColorProcessor {
public:
    ImgScaleWhite(ImgSource* parent, float amt);
    virtual ~ImgScaleWhite();
    virtual QColor doColorCorrection(QColor c) const;
private:
    float m_amt;
};

class ImgHueRot : public ImgColorProcessor {

public:
    ImgHueRot(ImgSource* parent, int amt);
    virtual ~ImgHueRot() ;
    virtual QColor doColorCorrection(QColor c) const;
private:
    int m_amt = 0;
};
class ImgHueInv : public ImgColorProcessor
{
public:
    ImgHueInv(ImgSource* parent);
    virtual ~ImgHueInv();
    virtual QColor doColorCorrection(QColor c) const;
};
class ImgHSVTweak : public ImgColorProcessor
{
  public:
    ImgHSVTweak(ImgSource* parent, int hmin, int hmax, int smin,
                       int smax, int vmin, int vmax, float hfact, int hconst, float sfact,
                       int sconst, float vfact, int vconst);
    virtual ~ImgHSVTweak();
    virtual QColor doColorCorrection(QColor c) const;
  private:
    int m_hmin, m_hmax,
        m_smin, m_smax,
        m_vmin, m_vmax,
        m_hconst, m_sconst, m_vconst;
    float m_hfact, m_sfact, m_vfact;
};
