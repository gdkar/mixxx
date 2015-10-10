_Pragma("once")
#include "engine/enginefilteriir.h"

class EngineFilterLinkwtzRiley8Low : public EngineFilterIIR<8, IIR_LP> {
    Q_OBJECT
  public:
    EngineFilterLinkwtzRiley8Low(int sampleRate, double freqCorner1,QObject*);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};
class EngineFilterLinkwtzRiley8High : public EngineFilterIIR<8, IIR_HP> {
    Q_OBJECT
  public:
    EngineFilterLinkwtzRiley8High(int sampleRate, double freqCorner1,QObject*);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};
