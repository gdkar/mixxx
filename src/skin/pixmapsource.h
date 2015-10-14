_Pragma("once")
#include <QString>
#include <QByteArray>

// A class representing an image source for a pixmap
// A bundle of a file path, raw data or inline svg
class PixmapSource {
  public:
    PixmapSource();
    PixmapSource(const QString& filepath);
    virtual ~PixmapSource();
    virtual bool isEmpty() const;
    virtual bool isSVG() const;
    virtual bool isBitmap() const;
    virtual void setSVG(const QByteArray& content);
    virtual void setPath(const QString& newPath);
    virtual QString getPath() const;
    virtual QByteArray getData() const;
    virtual QString getId() const;
  private:
    enum Type
    {
        SVG,
        BITMAP
    };
    QString m_path;
    QByteArray m_baData;
    enum Type m_eType;
};
