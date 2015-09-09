#include "sources/urlresource.h"


namespace Mixxx{
UrlResource::UrlResource(const QUrl& url ):m_url(url){}
bool UrlResource::isLocalFile()const{return getUrl().isLocalFile(); }
QString UrlResource::getLocalFileName() const{
  if(isLocalFile()) return getUrl().toLocalFile();
  else return QString{};
}
QByteArray UrlResource::getLocalFileNameBytes() const{return getLocalFileName().toLocal8Bit();}
QString UrlResource::getUrlString() const{return m_url.toString();}
const QUrl& UrlResource::getUrl()const{return m_url;}
};
