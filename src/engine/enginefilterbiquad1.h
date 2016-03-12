_Pragma("once")
#include "engine/enginefilteriir.h"

class EngineFilterBiquad1LowShelving : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1LowShelving(double sampleRate, double centerFreq, double Q);
    void setFrequencyCorners(double sampleRate, double centerFreq,double Q, double dBgain);
};
class EngineFilterBiquad1Peaking : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1Peaking(double sampleRate, double centerFreq, double Q);
    void setFrequencyCorners(double sampleRate, double centerFreq,double Q, double dBgain);
};
class EngineFilterBiquad1HighShelving : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1HighShelving(double sampleRate, double centerFreq, double Q);
    void setFrequencyCorners(double sampleRate, double centerFreq,double Q, double dBgain);
};
class EngineFilterBiquad1Low : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1Low(double sampleRate, double centerFreq, double Q,bool startFromDry);
    void setFrequencyCorners(double sampleRate, double centerFreq, double Q);
};
class EngineFilterBiquad1Band : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1Band(double sampleRate, double centerFreq, double Q);
    void setFrequencyCorners(double sampleRate, double centerFreq, double Q);
};
class EngineFilterBiquad1High : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1High(double sampleRate, double centerFreq, double Q,bool startFromDry);
    void setFrequencyCorners(double sampleRate, double centerFreq, double Q);
};
