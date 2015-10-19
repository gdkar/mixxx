#include "imgloader.h"
#include "widget/wwidget.h"

ImgLoader::ImgLoader() = default;
ImgLoader::~ImgLoader() = default;
QImage ImgLoader::getImage(QString img) const
{
    return QImage(img);
}

