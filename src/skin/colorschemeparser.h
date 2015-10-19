_Pragma("once")
#include "configobject.h"

class ImgSource;

class ColorSchemeParser {
  public:
    static void setupLegacyColorSchemes(QDomElement docElem, ConfigObject<ConfigValue>* pConfig);
  private:
    static ImgSource* parseFilters(QDomNode filter);
    ColorSchemeParser() = delete;
};
