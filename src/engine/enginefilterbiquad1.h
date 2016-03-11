_Pragma("once")
#include "engine/enginefilteriir.h"

#ifdef _MSC_VER
    // Visual Studio doesn't have snprintf
    #define format_fidspec sprintf_s
#else
    #define format_fidspec snprintf
#endif

class EngineFilterBiquad1LowShelving : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1LowShelving(int sampleRate, double centerFreq, double Q);
    void setFrequencyCorners(int sampleRate, double centerFreq,
                             double Q, double dBgain);

  private:
    char m_spec[FIDSPEC_LENGTH];
};

class EngineFilterBiquad1Peaking : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1Peaking(int sampleRate, double centerFreq, double Q);
    void setFrequencyCorners(int sampleRate, double centerFreq,
                             double Q, double dBgain);

  private:
    char m_spec[FIDSPEC_LENGTH];
};

class EngineFilterBiquad1HighShelving : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1HighShelving(int sampleRate, double centerFreq, double Q);
    void setFrequencyCorners(int sampleRate, double centerFreq,
                             double Q, double dBgain);

  private:
    char m_spec[FIDSPEC_LENGTH];
};

class EngineFilterBiquad1Low : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1Low(int sampleRate, double centerFreq, double Q,
                           bool startFromDry);
    void setFrequencyCorners(int sampleRate, double centerFreq, double Q);

  private:
    char m_spec[FIDSPEC_LENGTH];
};

class EngineFilterBiquad1Band : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1Band(int sampleRate, double centerFreq, double Q);
    void setFrequencyCorners(int sampleRate, double centerFreq, double Q);

  private:
    char m_spec[FIDSPEC_LENGTH];
};

class EngineFilterBiquad1High : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterBiquad1High(int sampleRate, double centerFreq, double Q,
                            bool startFromDry);
    void setFrequencyCorners(int sampleRate, double centerFreq, double Q);

  private:
    char m_spec[FIDSPEC_LENGTH];
};
