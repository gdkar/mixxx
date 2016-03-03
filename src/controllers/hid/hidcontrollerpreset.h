_Pragma("once")
#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetvisitor.h"
class HidControllerPreset : public ControllerPreset {
  public:
    HidControllerPreset()          = default;
    virtual ~HidControllerPreset() = default;
};
