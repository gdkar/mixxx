#include "engine/filter/enginefiltermoogladder4.h"


EngineFilterMoogLadder4Low::EngineFilterMoogLadder4Low(int sampleRate,
                                               double freqCorner1,
                                               double resonance, QObject *pParent)
        : EngineFilterMoogLadderBase(sampleRate, (float)freqCorner1, (float)resonance,pParent) {
}

EngineFilterMoogLadder4High::EngineFilterMoogLadder4High(int sampleRate,
                                                 double freqCorner1,
                                                 double resonance, QObject * pParent)
        : EngineFilterMoogLadderBase(sampleRate, (float)freqCorner1, (float)resonance,pParent) {
}


