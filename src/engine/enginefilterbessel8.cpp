#include "engine/enginefilterbessel8.h"
#include "util/math.h"


EngineFilterBessel8Low::EngineFilterBessel8Low(int sampleRate,double freqCorner1)
:EngineFilterIIR(nullptr, 8, IIR_LP)
{
    m_spec = "LpBe8";
    setFrequencyCorners(sampleRate, freqCorner1);
}
void EngineFilterBessel8Low::setFrequencyCorners(int sampleRate,double freqCorner1) {
    // Copy the old coefficients into m_oldCoef
    setCoefs(sampleRate, 8, "LpBe8", freqCorner1);
}
int EngineFilterBessel8Low::setFrequencyCornersForIntDelay(double desiredCorner1Ratio, int maxDelay) {
    // these values are calculated using the phase returned by
    // Fid::response_pha() at corner / 20

    // group delay at 1 Hz freqCorner1 and 1 Hz Samplerate
    const double kDelayFactor1 = 0.506051799;
    // Factor, required to hit the end of the quadratic curve
    const double kDelayFactor2 = 1.661247;
    // Table for the non quadratic, high part near the sample rate
    const double delayRatioTable[] = {
            0.500000000,  // delay 0
            0.321399282,  // delay 1
            0.213843537,  // delay 2
            0.155141284,  // delay 3
            0.120432232,  // delay 4
            0.097999886,  // delay 5
            0.082451739,  // delay 6
            0.071098408,  // delay 7
            0.062444910,  // delay 8
            0.055665936,  // delay 9
            0.050197933,  // delay 10
            0.045689120,  // delay 11
            0.041927420,  // delay 12
            0.038735202,  // delay 13
            0.035992756,  // delay 14
            0.033611618,  // delay 15
            0.031525020,  // delay 16
            0.029681641,  // delay 17
            0.028041409,  // delay 18
            0.026572562,  // delay 19
    };
    auto dDelay = kDelayFactor1 / desiredCorner1Ratio - kDelayFactor2 * desiredCorner1Ratio;
    auto iDelay =  static_cast<int>(math_clamp((int)(dDelay + 0.5), 0, maxDelay));
    double quantizedRatio;
    if (iDelay >= (int)(sizeof(delayRatioTable) / sizeof(double))) {
        // pq formula, only valid for low frequencies
        quantizedRatio = (-iDelay + std::sqrt(iDelay*iDelay + 4.0 * kDelayFactor1*kDelayFactor2)) / (2 * kDelayFactor2);
    } else {
        quantizedRatio = delayRatioTable[iDelay];
    }
    setCoefs(1., 8, "LpBe8", quantizedRatio );
    return iDelay;
}
EngineFilterBessel8Band::EngineFilterBessel8Band(int sampleRate,double freqCorner1,double freqCorner2)
:EngineFilterIIR(nullptr, 16, IIR_BP)
{
    m_spec = "BpBe8";
    setFrequencyCorners(sampleRate, freqCorner1, freqCorner2);
}
void EngineFilterBessel8Band::setFrequencyCorners(int sampleRate,double freqCorner1,double freqCorner2)
{
    setCoefs(sampleRate, 16, "BpBe8",  freqCorner1, freqCorner2);
}
EngineFilterBessel8High::EngineFilterBessel8High(int sampleRate,double freqCorner1)
:EngineFilterIIR(nullptr, 8, IIR_HP)
{
    setFrequencyCorners(sampleRate, freqCorner1);
}
void EngineFilterBessel8High::setFrequencyCorners(int sampleRate,double freqCorner1)
{
    m_spec = "HpBe8";
    setCoefs(sampleRate, 8, "HpBe8", freqCorner1);
}
