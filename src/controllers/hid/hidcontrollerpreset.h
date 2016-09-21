#ifndef HIDCONTROLLERPRESET_H
#define HIDCONTROLLERPRESET_H

#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetvisitor.h"

class HidControllerPreset : public ControllerPreset {
  public:
    HidControllerPreset() = default;
    virtual ~HidControllerPreset() = default;
};

#endif /* HIDCONTROLLERPRESET_H */
