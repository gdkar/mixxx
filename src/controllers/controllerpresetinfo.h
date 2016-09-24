/**
* @file controllerpresetinfo.h
* @author Ilkka Tuohela hile@iki.fi
* @date Wed May 15 2012
* @brief Base class handling enumeration and parsing of preset info headers
*
* This class handles enumeration and parsing of controller XML description file
* <info> header tags. It can be used to match controllers automatically or to
* show details for a mapping.
*/

#ifndef CONTROLLERPRESETINFO_H
#define CONTROLLERPRESETINFO_H

#include <QString>
#include <QMap>
#include <QList>
#include <QDomElement>

#include "preferences/usersettings.h"
#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetfilehandler.h"

class PresetInfo {
  public:
    PresetInfo();
    PresetInfo(const QString& path);
    bool isValid() const {
        return m_valid;
    }
    QString getPath() const { return m_path; }
    QString getDeviceName() const { return m_name; }
    QString getDescription() const { return m_description; }
    QString getForumLink() const { return m_forumlink; }
    QString getWikiLink() const { return m_wikilink; }
    QString getAuthor() const { return m_author; }
    const QList<ProductInfo>& getProducts() const { return m_products; }
  private:
    ProductInfo parseBulkProduct(const QDomElement& element) const;
    ProductInfo parseHIDProduct(const QDomElement& element) const;

    bool m_valid;
    QString m_path;
    QString m_name;
    QString m_author;
    QString m_description;
    QString m_forumlink;
    QString m_wikilink;
    QList<ProductInfo> m_products;
};

#endif
