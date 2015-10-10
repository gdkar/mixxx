_Pragma("once")
#include "engine/enginefilteriir.h"
#include <cstring>
#define format_fidspec std::snprintf

class EngineFilterBiquad1LowShelving : public EngineFilterIIR<5, IIR_BP> {
    Q_OBJECT
  public:
    EngineFilterBiquad1LowShelving(int sampleRate, double centerFreq, double Q,QObject*);
    void setFrequencyCorners(int sampleRate, double centerFreq,double Q, double dBgain);
  private:
    char m_spec[FIDSPEC_LENGTH];
};
class EngineFilterBiquad1Peaking : public EngineFilterIIR<5, IIR_BP> {
    Q_OBJECT
  public:
    EngineFilterBiquad1Peaking(int sampleRate, double centerFreq, double Q,QObject*);
    void setFrequencyCorners(int sampleRate, double centerFreq,double Q, double dBgain);
  private:
    char m_spec[FIDSPEC_LENGTH];
};
class EngineFilterBiquad1HighShelving : public EngineFilterIIR<5, IIR_BP> {
    Q_OBJECT
  public:
    EngineFilterBiquad1HighShelving(int sampleRate, double centerFreq, double Q,QObject*);
    void setFrequencyCorners(int sampleRate, double centerFreq,double Q, double dBgain);
  private:
    char m_spec[FIDSPEC_LENGTH];
};
class EngineFilterBiquad1Low : public EngineFilterIIR<2, IIR_LP> {
    Q_OBJECT
  public:
    EngineFilterBiquad1Low(int sampleRate, double centerFreq, double Q,bool startFromDry,QObject*);
    void setFrequencyCorners(int sampleRate, double centerFreq, double Q);
  private:
    char m_spec[FIDSPEC_LENGTH];
};
class EngineFilterBiquad1Band : public EngineFilterIIR<2, IIR_BP> {
    Q_OBJECT
  public:
    EngineFilterBiquad1Band(int sampleRate, double centerFreq, double Q,QObject*);
    void setFrequencyCorners(int sampleRate, double centerFreq, double Q);
  private:
    char m_spec[FIDSPEC_LENGTH];
};
class EngineFilterBiquad1High : public EngineFilterIIR<2, IIR_HP> {
    Q_OBJECT
  public:
    EngineFilterBiquad1High(int sampleRate, double centerFreq, double Q,bool startFromDry,QObject*);
    void setFrequencyCorners(int sampleRate, double centerFreq, double Q);
  private:
    char m_spec[FIDSPEC_LENGTH];
};
