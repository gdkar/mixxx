/**
* @file controllerpresetinfo.cpp
* @author Ilkka Tuohela hile@iki.fi
* @date Wed May 15 2012
* @brief Implement handling enumeration and parsing of preset info headers
*
* This class handles parsing of controller XML description file
* <info> header tags. It can be used to match controllers automatically or to
* show details for a mapping.
*/

#include "controllers/controllerpresetinfo.h"
#include "controllers/controllerpresetinfoenumerator.h"

#include "controllers/defs_controllers.h"
#include "util/xml.h"

PresetInfo::PresetInfo() = default;
PresetInfo::~PresetInfo() = default;

PresetInfo::PresetInfo(const QString preset_path)
{
    // Parse <info> header section from a controller description XML file
    // Contents parsed by xml path:
    // info.name        Preset name, used for drop down menus in dialogs
    // info.author      Preset author
    // info.description Preset description
    // info.forums      Link to mixxx forum discussion for the preset
    // info.wiki        Link to mixxx wiki for the preset
    // info.devices.product List of device matches, specific to device type
    m_path = QFileInfo(preset_path).absoluteFilePath();
    m_name = "";
    m_author = "";
    m_description = "";
    m_forumlink = "";
    m_wikilink = "";
    auto root = XmlParse::openXMLFile(m_path, "controller");
    if (root.isNull())
    {
        qDebug() << "ERROR parsing" << m_path;
        return;
    }
    auto info = root.firstChildElement("info");
    if (info.isNull())
    {
        qDebug() << "MISSING <info> ELEMENT: " << m_path;
        return;
    }
    m_valid = true;
    auto dom_name = info.firstChildElement("name");
    if (!dom_name.isNull()) m_name = dom_name.text();
    auto dom_author = info.firstChildElement("author");
    if (!dom_author.isNull()) m_author = dom_author.text();
    auto dom_description = info.firstChildElement("description");
    if (!dom_description.isNull()) m_description = dom_description.text();
    auto dom_forums = info.firstChildElement("forums");
    if (!dom_forums.isNull()) m_forumlink = dom_forums.text();
    auto dom_wiki = info.firstChildElement("wiki");
    if (!dom_wiki.isNull()) m_wikilink = dom_wiki.text();
    auto devices = info.firstChildElement("devices");
    if (!devices.isNull())
    {
        auto product = devices.firstChildElement("product");
        while (!product.isNull())
        {
            auto protocol = product.attribute("protocol","");
            if (protocol=="hid") m_products.append(parseHIDProduct(product));
            else if (protocol=="bulk") m_products.append(parseBulkProduct(product));
            else if (protocol=="midi") qDebug("MIDI product info parsing not yet implemented");
            else if (protocol=="osc")  qDebug("OSC product info parsing not yet implemented");
            else qDebug("Product specification missing protocol attribute");
            product = product.nextSiblingElement("product");
        }
    }
}
QHash<QString,QString> PresetInfo::parseBulkProduct(const QDomElement& element) const
{
    // <product protocol="bulk" vendor_id="0x06f8" product_id="0x0b105" in_epaddr="0x82" out_epaddr="0x03">
    QHash<QString, QString> product;
    product.insert("protocol", element.attribute("protocol",""));
    product.insert("vendor_id", element.attribute("vendor_id",""));
    product.insert("product_id", element.attribute("product_id",""));
    product.insert("in_epaddr", element.attribute("in_epaddr",""));
    product.insert("out_epaddr", element.attribute("out_epaddr",""));
    return product;
}
QHash<QString,QString> PresetInfo::parseHIDProduct(const QDomElement& element) const
{
    // HID device <product> element parsing. Example of valid element:
    //   <product protocol="hid" vendor_id="0x1" product_id="0x2" usage_page="0x3" usage="0x4" interface_number="0x3" />
    // All numbers must be hex prefixed with 0x
    // Only vendor_id and product_id fields are required to map a device.
    // usage_page and usage are matched on OS/X and windows
    // interface_number is matched on linux, which does support usage_page/usage

    QHash<QString,QString> product;
    product.insert("procotol", element.attribute("protocol",""));
    product.insert("vendor_id", element.attribute("vendor_id",""));
    product.insert("product_id", element.attribute("product_id",""));
    product.insert("usage_page", element.attribute("usage_page",""));
    product.insert("usage", element.attribute("usage",""));
    product.insert("interface_number", element.attribute("interface_number",""));
    return product;
}
QHash<QString,QString> PresetInfo::parseMIDIProduct(const QDomElement& element) const
{
    // TODO - implement parsing of MIDI attributes
    // When done, remember to fix switch() above to call this
    QHash<QString,QString> product;
    product.insert("procotol",element.attribute("protocol",""));
    return product;
}
QHash<QString,QString> PresetInfo::parseOSCProduct(const QDomElement& element) const
{
    // TODO - implement parsing of OSC product attributes
    // When done, remember to fix switch() above to call this
    QHash<QString,QString> product;
    product.insert("procotol",element.attribute("protocol",""));
    return product;
}
