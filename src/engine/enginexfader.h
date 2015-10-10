_Pragma("once")
// HACK until we have Control 2.0
#define MIXXX_XFADER_ADDITIVE   0.0
#define MIXXX_XFADER_CONSTPWR   1.0

class EngineXfader {
  public:
    static double getCalibration(double transform);
    static void getXfadeGains(
        double xfadePosition, double transform, double calibration,
        bool constPower, bool reverse, double* gain1, double* gain2);
};
