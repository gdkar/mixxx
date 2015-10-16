_Pragma("once")
#include "widget/woverview.h"

class WOverviewLMH : public WOverview {
  public:
    WOverviewLMH(const char *pGroup, ConfigObject<ConfigValue>* pConfig, QWidget* parent);
    virtual ~WOverviewLMH();
  private:
    virtual bool drawNextPixmapPart();
};
