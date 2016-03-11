_Pragma("once")
#include "engine/enginefilteriir.h"

class EngineFilterLinkwtzRiley8Low : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterLinkwtzRiley8Low(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};


class EngineFilterLinkwtzRiley8High : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterLinkwtzRiley8High(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};
