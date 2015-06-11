#ifndef ENGINEXFADER_H
#define ENGINEXFADER_H

#include "util/types.h"
#include "util/math.h"

// HACK until we have Control 2.0
#define MIXXX_XFADER_ADDITIVE   0.0
#define MIXXX_XFADER_CONSTPWR   1.0

class EngineXfader {
  public:
    static double getCalibration(double transform);
    static void getXfadeGains(
        double xfadePosition, double transform, double calibration,
        bool constPower, bool reverse, CSAMPLE_GAIN &gain1,CSAMPLE_GAIN &gain2);
};

#endif /* ENGINEXFADER_H */
