#include "engine/enginefilterbutterworth4.h"


EngineFilterButterworth4Low::EngineFilterButterworth4Low(int sampleRate, double freqCorner1)
:EngineFilterIIR(4,IIR_LP)
{
    m_spec = "LpBu4";
    setFrequencyCorners(sampleRate, freqCorner1);
}

void EngineFilterButterworth4Low::setFrequencyCorners(int sampleRate,
                                             double freqCorner1) {
    // Copy the old coefficients into m_oldCoef
    setCoefs(sampleRate, 4, "LpBu4", freqCorner1);
}


EngineFilterButterworth4Band::EngineFilterButterworth4Band(int sampleRate, double freqCorner1,
                                         double freqCorner2)
:EngineFilterIIR(8,IIR_BP)
{
    m_spec = "BpBu4";
    setFrequencyCorners(sampleRate, freqCorner1, freqCorner2);
}

void EngineFilterButterworth4Band::setFrequencyCorners(int sampleRate,
                                             double freqCorner1,
                                             double freqCorner2) {
    setCoefs(sampleRate, 8, "BpBu4", freqCorner1, freqCorner2);
}


EngineFilterButterworth4High::EngineFilterButterworth4High(int sampleRate, double freqCorner1)
:EngineFilterIIR(4,IIR_HP)
{
    setFrequencyCorners(sampleRate, freqCorner1);
}

void EngineFilterButterworth4High::setFrequencyCorners(int sampleRate,
                                             double freqCorner1) {
    m_spec = "HpBu4";
    setCoefs(sampleRate, 4, "HpBu4", freqCorner1);
}
