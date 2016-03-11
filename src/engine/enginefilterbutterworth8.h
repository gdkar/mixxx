_Pragma("once")
#include "engine/enginefilteriir.h"

class EngineFilterButterworth8Low : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterButterworth8Low(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};

class EngineFilterButterworth8Band : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterButterworth8Band(int sampleRate, double freqCorner1,
            double freqCorner2);
    void setFrequencyCorners(int sampleRate, double freqCorner1,
            double freqCorner2);
};

class EngineFilterButterworth8High : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterButterworth8High(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};

