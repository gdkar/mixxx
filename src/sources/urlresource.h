#ifndef MIXXX_URLRESOURCE_H
#define MIXXX_URLRESOURCE_H
#include "util/assert.h"
#include <QUrl>
namespace Mixxx {
class UrlResource {
    const QUrl m_url;
public:
    virtual ~UrlResource() = default;
    const QUrl& getUrl() const;
    QString getUrlString() const;
protected:
    explicit UrlResource(const QUrl& url);
    virtual bool isLocalFile() const;
    virtual QString getLocalFileName() const; 
    virtual QByteArray getLocalFileNameBytes() const;
};
} // namespace Mixxx
#endif // MIXXX_URLRESOURCE_H
