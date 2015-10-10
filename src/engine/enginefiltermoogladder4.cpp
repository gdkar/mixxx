#include "engine/enginefiltermoogladder4.h"


EngineFilterMoogLadder4Low::EngineFilterMoogLadder4Low(int sampleRate,
                                               double freqCorner1,
                                               double resonance,
                                               QObject *p)
        : EngineFilterMoogLadderBase(sampleRate, (float)freqCorner1, (float)resonance,p) {
}

EngineFilterMoogLadder4High::EngineFilterMoogLadder4High(int sampleRate,
                                                 double freqCorner1,
                                                 double resonance,
                                                 QObject *p)
        : EngineFilterMoogLadderBase(sampleRate, (float)freqCorner1, (float)resonance,p) {
}


