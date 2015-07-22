#ifndef HIDCONTROLLERPRESET_H
#define HIDCONTROLLERPRESET_H

#include "controllers/controllerpreset.h"
#include "controllers/controllerpresetvisitor.h"

class HidControllerPreset : public ControllerPreset {
  public:
    HidControllerPreset() {}
    virtual ~HidControllerPreset() {}
    virtual bool isMappable() const {
        return false;
    }
};

#endif /* HIDCONTROLLERPRESET_H */
