#include "skin/imgsource.h"

ImgSource::~ImgSource() = default;
QColor ImgSource::getCorrectColor(QColor c) const
{
  return c;
}
QImage ImgSource::correctImageColors(QImage i) const
{
  return i;
}
ImgProcessor::ImgProcessor(ImgSource *p) : m_parent(p)
{
}
ImgProcessor::~ImgProcessor() = default;
QColor ImgProcessor::getCorrectColor(QColor c) const
{
  return doColorCorrection(m_parent->getCorrectColor(c));
}
QImage ImgProcessor::correctImageColors(QImage i ) const
{
  return i;
}
ImgColorProcessor::~ImgColorProcessor() = default;
ImgColorProcessor::ImgColorProcessor(ImgSource *p) : ImgProcessor(p)
{
}
QImage ImgColorProcessor::getImage(QString img) const
{
  return correctImageColors(m_parent->getImage(img));
}
QImage ImgColorProcessor::correctImageColors(QImage i) const
{
    if (i.isNull())return i;
    auto bytesPerPixel = 4;
    switch(i.format())
    {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Indexed8:
        bytesPerPixel = 1;
        break;
    case QImage::Format_RGB16:
    case QImage::Format_RGB555:
    case QImage::Format_RGB444:
    case QImage::Format_ARGB4444_Premultiplied:
        bytesPerPixel = 2;
        break;
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_RGB666:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_RGB888:
        bytesPerPixel = 3;
        break;
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_RGB32:
        bytesPerPixel = 4;
        break;
    case QImage::Format_Invalid:
    default:
        bytesPerPixel = 0;
        break;
    }
    auto ret = QImage{};
    if (bytesPerPixel < 4)
    {
        // Handling Indexed color or mono colors requires different logic
        qDebug() << "ImgColorProcessor converting unsupported color format:" << i.format();
        ret = i.convertToFormat(QImage::Format_ARGB32);
    }
    else
    {
      ret = i;
    }
    for (auto y = 0; y < ret.height(); y++)
    {
        auto line = reinterpret_cast<QRgb*>(ret.scanLine(y)); // cast the returned pointer to QRgb*
        if (!line ) continue;
        for (auto x = 0; x < ret.width(); x++,line++) *line = doColorCorrection(QColor(*line)).rgba();
    }
    return ret;
}
