_Pragma("once")
#include "widget/woverview.h"

class WOverviewRGB : public WOverview {
  public:
    WOverviewRGB(const char *pGroup, ConfigObject<ConfigValue>* pConfig, QWidget* parent);
    virtual ~WOverviewRGB();
  private:
    virtual bool drawNextPixmapPart();
};
