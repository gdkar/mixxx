#include "widget/wimagestore.h"

#include <QtDebug>
#include <QSvgRenderer>
#include <QPainter>

// static
QHash<QString, WImageStore::ImageRef> WImageStore::m_dictionary;
QSharedPointer<ImgSource> WImageStore::m_loader = QSharedPointer<ImgSource>();

// static
QImage* WImageStore::getImageNoCache(const QString& fileName)
{
    return getImageNoCache(PixmapSource(fileName));
}

// static
QImage* WImageStore::getImage(const QString& fileName)
{
    return getImage(PixmapSource(fileName));
}

// static
QImage* WImageStore::getImage(const PixmapSource& source)
{
    // Search for Image in list
    if ( m_dictionary.contains(source.getId()))
    {
      auto info = m_dictionary.value(source.getId());
      info.refcnt++;
      return info.image;
    }
    // Image wasn't found, construct it
    //qDebug() << "WImageStore Loading Image from file" << source.getPath();
    auto loadedImage = getImageNoCache(source);
    if (!loadedImage ) return nullptr;
    if (loadedImage->isNull())
    {
        qDebug() << "WImageStore couldn't load:" << source.getPath() << (loadedImage == NULL);
        delete loadedImage;
        return nullptr;
    }
    auto info  = ImageRef{};
    info.image = loadedImage;
    info.refcnt= 1;
    m_dictionary.insert(source.getId(), info);
    return info.image;
}
// static
QImage* WImageStore::getImageNoCache(const PixmapSource& source)
{
    auto pImage = static_cast<QImage*>(nullptr);
    if (source.isSVG())
    {
        QSvgRenderer renderer;
        if (source.getData().isEmpty()) renderer.load(source.getPath());
        else                            renderer.load(source.getData());
        pImage = new QImage(renderer.defaultSize(), QImage::Format_ARGB32);
        pImage->fill(0x00000000);  // Transparent black.
        QPainter painter(pImage);
        renderer.render(&painter);
    } else {
        if (m_loader) pImage = m_loader->getImage(source.getPath());
        else          pImage = new QImage(source.getPath());
    }
    return pImage;
}
// static
void WImageStore::deleteImage(QImage * p)
{
    // Search for Image in list
    QMutableHashIterator<QString, ImageRef> it(m_dictionary);
    while (it.hasNext())
    {
        auto &info = it.next().value();
        if (p == info.image)
        {
            info.refcnt--;
            if (info.refcnt<1)
            {
                delete info.image;
                it.remove();
            }
            break;
        }
    }
}
// static
void WImageStore::correctImageColors(QImage* p)
{
    if (m_loader) m_loader->correctImageColors(p);
}
// static
void WImageStore::setLoader(QSharedPointer<ImgSource> ld)
{
    m_loader = ld;
}
