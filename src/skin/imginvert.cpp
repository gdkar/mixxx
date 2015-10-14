#include "imginvert.h"
ImgInvert::ImgInvert(ImgSource *p) 
: ImgColorProcessor(p)
{
}
ImgInvert::~ImgInvert() = default;
QColor ImgInvert::doColorCorrection(QColor c) const
{
    return QColor(0xff - c.red(), 0xff - c.green(), 0xff - c.blue(), c.alpha());
}

