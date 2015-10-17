
#include "skin/colorschemeparser.h"

#include "widget/wpixmapstore.h"
#include "widget/wimagestore.h"
#include "widget/wskincolor.h"
#include "widget/wwidget.h"
#include "util/xml.h"
#include "skin/imgsource.h"
#include "skin/imgloader.h"
#include "skin/imgcolor.h"
#include "skin/imginvert.h"

void ColorSchemeParser::setupLegacyColorSchemes(QDomElement docElem,ConfigObject<ConfigValue>* pConfig)
{
    auto colsch = docElem.namedItem("Schemes");
    if (!colsch.isNull() && colsch.isElement())
    {
        auto schname = pConfig->getValueString(ConfigKey("Config","Scheme"));
        auto sch = colsch.firstChild();
        auto found = false;
        if (schname.isEmpty())
        {
            // If no scheme stored, accept the first one in the file
            found = true;
        }
        while (!sch.isNull() && !found)
        {
            auto thisname = XmlParse::selectNodeQString(sch, "Name");
            if (thisname == schname) found = true;
            else sch = sch.nextSibling();
        }
        if (found)
        {
            auto imsrc = QSharedPointer<ImgSource>(parseFilters(sch.namedItem("Filters")));
            WPixmapStore::setLoader(imsrc);
            WImageStore::setLoader(imsrc);
            WSkinColor::setLoader(imsrc);
        }
        else
        {
            WPixmapStore::setLoader(QSharedPointer<ImgSource>());
            WImageStore::setLoader(QSharedPointer<ImgSource>());
            WSkinColor::setLoader(QSharedPointer<ImgSource>());
        }
    }
    else
    {
        WPixmapStore::setLoader(QSharedPointer<ImgSource>());
        WImageStore::setLoader(QSharedPointer<ImgSource>());
        WSkinColor::setLoader(QSharedPointer<ImgSource>());
    }
}
ImgSource* ColorSchemeParser::parseFilters(QDomNode filt)
{
    // TODO: Move this code into ImgSource
    if (!filt.hasChildNodes()) return 0;
    auto ret = static_cast<ImgSource*>(new ImgLoader());
    auto f = filt.firstChild();
    while (!f.isNull())
    {
        auto name = f.nodeName().toLower();
        if (name == "invert")          ret = new ImgInvert(ret);
        else if (name == "hueinv")     ret = new ImgHueInv(ret);
        else if (name == "add")        ret = new ImgAdd(ret, XmlParse::selectNodeInt(f, "Amount"));
        else if (name == "scalewhite") ret = new ImgScaleWhite(ret, XmlParse::selectNodeFloat(f, "Amount"));
        else if (name == "hsvtweak")
        {
            auto hmin = 0;
            auto hmax = 359;
            auto smin = 0;
            auto smax = 255;
            auto vmin = 0;
            auto vmax = 255;
            auto hfact = 1.0f;
            auto sfact = 1.0f;
            auto vfact = 1.0f;
            auto hconst = 0;
            auto sconst = 0;
            auto vconst = 0;

            if (!f.namedItem("HMin").isNull()) { hmin = XmlParse::selectNodeInt(f, "HMin"); }
            if (!f.namedItem("HMax").isNull()) { hmax = XmlParse::selectNodeInt(f, "HMax"); }
            if (!f.namedItem("SMin").isNull()) { smin = XmlParse::selectNodeInt(f, "SMin"); }
            if (!f.namedItem("SMax").isNull()) { smax = XmlParse::selectNodeInt(f, "SMax"); }
            if (!f.namedItem("VMin").isNull()) { vmin = XmlParse::selectNodeInt(f, "VMin"); }
            if (!f.namedItem("VMax").isNull()) { vmax = XmlParse::selectNodeInt(f, "VMax"); }

            if (!f.namedItem("HConst").isNull()) { hconst = XmlParse::selectNodeInt(f, "HConst"); }
            if (!f.namedItem("SConst").isNull()) { sconst = XmlParse::selectNodeInt(f, "SConst"); }
            if (!f.namedItem("VConst").isNull()) { vconst = XmlParse::selectNodeInt(f, "VConst"); }

            if (!f.namedItem("HFact").isNull()) { hfact = XmlParse::selectNodeFloat(f, "HFact"); }
            if (!f.namedItem("SFact").isNull()) { sfact = XmlParse::selectNodeFloat(f, "SFact"); }
            if (!f.namedItem("VFact").isNull()) { vfact = XmlParse::selectNodeFloat(f, "VFact"); }
            ret = new ImgHSVTweak(ret, hmin, hmax, smin, smax, vmin, vmax, hfact, hconst,sfact,sconst,vfact,vconst);
        }
        else qDebug() << "Unkown image filter:" << name;
        f = f.nextSibling();
    }
    return ret;
}
