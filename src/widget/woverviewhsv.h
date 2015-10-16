_Pragma("once")
#include "widget/woverview.h"

class WOverviewHSV : public WOverview {
  public:
    WOverviewHSV(const char *pGroup, ConfigObject<ConfigValue>* pConfig, QWidget* parent);
    virtual ~WOverviewHSV();
  private:
    virtual bool drawNextPixmapPart();
};
