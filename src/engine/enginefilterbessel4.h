_Pragma("once")
#include "engine/enginefilteriir.h"

class EngineFilterBessel4Low : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBessel4Low(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
    // This function selects a corner frequency near the
    // desiredCorner1Ratio freqCorner / sampleRate
    // the produces an integer group delay at the passband
    // Optimized for freqCorner / 20
    int setFrequencyCornersForIntDelay(double desiredCorner1Ratio, int maxDelay);
};

class EngineFilterBessel4Band : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBessel4Band(int sampleRate, double freqCorner1,
            double freqCorner2);
    void setFrequencyCorners(int sampleRate, double freqCorner1,
            double freqCorner2);
};

class EngineFilterBessel4High : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBessel4High(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};

