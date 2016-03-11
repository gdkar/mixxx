_Pragma("once")
#include "engine/enginefilteriir.h"

class EngineFilterLinkwtzRiley4Low : public EngineFilterIIR {
    Q_OBJECT
  public:
    EngineFilterLinkwtzRiley4Low(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};


class EngineFilterLinkwtzRiley4High : public EngineFilterIIR {
    Q_OBJECT
  public:
    EngineFilterLinkwtzRiley4High(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};
