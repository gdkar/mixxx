_Pragma("once")
#include <QHash>
#include <QSharedPointer>

#include "skin/imgsource.h"
#include "skin/pixmapsource.h"

class QImage;

class WImageStore {
  public:
    static QImage getImage(const QString &fileName);
    static QImage getImageNoCache(const QString &fileName);
    static QImage getImage(const PixmapSource& source);
    static QImage getImageNoCache(const PixmapSource& source);
    static void deleteImage(QImage p);
    static void setLoader(QSharedPointer<ImgSource> ld);
    // For external owned images like software generated ones.
    struct ImageRef {
      QImage  image;
      int64_t refcnt = 1;
    };
  private:
    /** Dictionary of Images already instantiated */
    static QHash<QString, ImageRef> m_dictionary;
    static QSharedPointer<ImgSource> m_loader;
};
Q_DECLARE_TYPEINFO(WImageStore::ImageRef,Q_PRIMITIVE_TYPE);
