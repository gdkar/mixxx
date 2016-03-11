#include "engine/enginefilterbutterworth8.h"

EngineFilterButterworth8Low::EngineFilterButterworth8Low(int sampleRate, double freqCorner1)
:EngineFilterIIR(8,IIR_LP)
{
    m_spec = "LpBu8";
    setFrequencyCorners(sampleRate, freqCorner1);
}

void EngineFilterButterworth8Low::setFrequencyCorners(int sampleRate,
                                             double freqCorner1) {
    // Copy the old coefficients into m_oldCoef
    setCoefs(sampleRate, 8, "LpBu8", freqCorner1);
}


EngineFilterButterworth8Band::EngineFilterButterworth8Band(int sampleRate, double freqCorner1,
                                         double freqCorner2)
:EngineFilterIIR(16,IIR_BP)
{
    m_spec = "BpBu8";
    setFrequencyCorners(sampleRate, freqCorner1, freqCorner2);
}

void EngineFilterButterworth8Band::setFrequencyCorners(int sampleRate,
                                             double freqCorner1,
                                             double freqCorner2) {
    setCoefs(sampleRate, 16, "BpBu8", freqCorner1, freqCorner2);
}

EngineFilterButterworth8High::EngineFilterButterworth8High(int sampleRate, double freqCorner1)
:EngineFilterIIR(8,IIR_HP)
{
    setFrequencyCorners(sampleRate, freqCorner1);
}

void EngineFilterButterworth8High::setFrequencyCorners(int sampleRate,
                                             double freqCorner1)
{
    m_spec = "HpBu8";
    setCoefs(sampleRate, 8, "HpBu8", freqCorner1);
}
