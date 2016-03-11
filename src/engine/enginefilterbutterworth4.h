_Pragma("once")
#include "engine/enginefilteriir.h"

class EngineFilterButterworth4Low : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterButterworth4Low(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};

class EngineFilterButterworth4Band : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterButterworth4Band(int sampleRate, double freqCorner1,
            double freqCorner2);
    void setFrequencyCorners(int sampleRate, double freqCorner1,
            double freqCorner2);
};

class EngineFilterButterworth4High : public EngineFilterIIR{
    Q_OBJECT
  public:
    EngineFilterButterworth4High(int sampleRate, double freqCorner1);
    void setFrequencyCorners(int sampleRate, double freqCorner1);
};
