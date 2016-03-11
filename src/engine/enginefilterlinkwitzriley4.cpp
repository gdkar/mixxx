#include "engine/enginefilterlinkwitzriley4.h"


EngineFilterLinkwtzRiley4Low::EngineFilterLinkwtzRiley4Low(int sampleRate, double freqCorner1)
:EngineFilterIIR(4,IIR_LP)
{
    m_spec = "HpBu2xHpBu2";
    setFrequencyCorners(sampleRate, freqCorner1);
}

void EngineFilterLinkwtzRiley4Low::setFrequencyCorners(int sampleRate,
                                             double freqCorner1) {
    // Copy the old coefficients into m_oldCoef
    setCoefs2(sampleRate,
            2,"LpBu2", freqCorner1, 0, 0,
            2,"LpBu2", freqCorner1, 0, 0);
}

EngineFilterLinkwtzRiley4High::EngineFilterLinkwtzRiley4High(int sampleRate, double freqCorner1)
:EngineFilterIIR(4,IIR_HP){
    setFrequencyCorners(sampleRate, freqCorner1);
}

void EngineFilterLinkwtzRiley4High::setFrequencyCorners(int sampleRate,
                                             double freqCorner1) {

    m_spec = "HpBu2xHpBu2";
    setCoefs2(sampleRate,
            2,"HpBu2", freqCorner1, 0, 0,
            2,"HpBu2", freqCorner1, 0, 0);
}
