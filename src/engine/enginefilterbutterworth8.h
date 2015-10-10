_Pragma("once")
#include "engine/enginefilteriir.h"

class EngineFilterButterworth8Low : public EngineFilterIIR<8, IIR_LP> {
    Q_OBJECT
  public:
    EngineFilterButterworth8Low(int sampleRate, double freqCorner1,QObject*);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};
class EngineFilterButterworth8Band : public EngineFilterIIR<16, IIR_BP> {
    Q_OBJECT
  public:
    EngineFilterButterworth8Band(int sampleRate, double freqCorner1,double freqCorner2,QObject*);
    void setFrequencyCorners(int sampleRate, double freqCorner1,double freqCorner2);
};
class EngineFilterButterworth8High : public EngineFilterIIR<8, IIR_HP> {
    Q_OBJECT
  public:
    EngineFilterButterworth8High(int sampleRate, double freqCorner1,QObject*);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};
