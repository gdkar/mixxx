#include "effects/native/lvmixeqbase.h"

namespace {
    const int kMaxDelay = 3300; // allows a 30 Hz filter at 97346;
    const unsigned int kStartupSamplerate = 44100;
    const double kStartupLoFreq = 246;
    const double kStartupHiFreq = 2484;

    const double kf1Tab[] = { -1, -1, -1, -1, 0.336440447, -1,-1, -1, 0.506051799 };
    const double kf2Tab[] = { -1, -1, -1, -1, 1.1044845,   -1,-1, -1, 1.661247    };
    const double dr8Tab[] = {
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
    const double dr4Tab[] = {
            0.500000000,  // delay 0
            0.258899546,  // delay 1
            0.154778862,  // delay 2
            0.107833769,  // delay 3
            0.082235025,  // delay 4
            0.066314175,  // delay 5
            0.055505336,  // delay 6
            0.047691446,  // delay 7
            0.041813481,  // delay 8
            0.037212241,  // delay 9
            0.033519902,  // delay 10
            0.030497945,  // delay 11
            0.027964718,  // delay 12
    };
    const double * drTab[] = {nullptr, nullptr, nullptr, nullptr, dr4Tab, nullptr, nullptr, nullptr, dr8Tab};
    const int drTabSize[] = { 0, 0, 0, 0, sizeof(dr4Tab)/sizeof(dr4Tab[0]), 0, 0, 0, sizeof(dr8Tab)/sizeof(dr8Tab[0])};
    int setFrequencyCornersForIntDelay(std::unique_ptr<EngineFilterIIR> &filt, size_t size, double desiredCorner1Ratio, int maxDelay)
    {

        // these values are calculated using the phase returned by
        // Fid::response_pha() at corner / 20

        // group delay at 1 Hz freqCorner1 and 1 Hz Samplerate
        if(size > sizeof(kf1Tab) / sizeof(kf1Tab[0]))
            return 0;
        auto kDelayFactor1 = kf1Tab[size];
        auto kDelayFactor2 = kf2Tab[size];
        auto dDelay = kDelayFactor1 / desiredCorner1Ratio - kDelayFactor2 * desiredCorner1Ratio;
        auto iDelay =  static_cast<int>(math_clamp((int)(dDelay + 0.5), 0, maxDelay));
        auto delayRatioTable = drTab[size];
        auto tabSize = drTabSize[size];
        double quantizedRatio;
        if (iDelay >= tabSize) {
            // pq formula, only valid for low frequencies
            quantizedRatio = (-iDelay + std::sqrt(iDelay*iDelay + 4.0 * kDelayFactor1*kDelayFactor2)) / (2 * kDelayFactor2);
        } else {
            quantizedRatio = delayRatioTable[iDelay];
        }
        filt->setFrequencyCorners(1., quantizedRatio );
        return iDelay;
    }
}

LVMixEQEffectGroupState::LVMixEQEffectGroupState(size_t size)
    : m_size(size),
        m_oldLow(1.0),
        m_oldMid(1.0),
        m_oldHigh(1.0),
        m_oldSampleRate(kStartupSamplerate),
        m_loFreq(kStartupLoFreq),
        m_hiFreq(kStartupHiFreq)
{

    m_pLowBuf = std::make_unique<CSAMPLE[]>(MAX_BUFFER_LEN);
    m_pBandBuf = std::make_unique<CSAMPLE[]>(MAX_BUFFER_LEN);
    m_pHighBuf = std::make_unique<CSAMPLE[]>(MAX_BUFFER_LEN);

    m_low1 = std::make_unique<EngineFilterIIR>(m_size, EngineFilterIIR::LowPass, QString{"LpBe%1"}.arg(m_size));//kStartupSamplerate, kStartupLoFreq);
    m_low2 = std::make_unique<EngineFilterIIR>(m_size, EngineFilterIIR::LowPass, QString{"LpBe%1"}.arg(m_size));//kStartupSamplerate, kStartupLoFreq);

    m_delay2 = std::make_unique<EngineFilterDelay>(nullptr, kMaxDelay);
    m_delay3 = std::make_unique<EngineFilterDelay>(nullptr, kMaxDelay);

    setFilters(kStartupSamplerate, kStartupLoFreq, kStartupHiFreq);
}

LVMixEQEffectGroupState::~LVMixEQEffectGroupState() = default;

void LVMixEQEffectGroupState::setFilters(int sampleRate, double lowFreq, double highFreq)
{
    auto loDelay = m_loDelay;
    auto hiDelay = m_hiDelay;
    if(m_oldSampleRate != sampleRate || lowFreq != m_loFreq) {
        loDelay = setFrequencyCornersForIntDelay( m_low1, m_size, lowFreq / sampleRate, kMaxDelay);
    }
    if(m_oldSampleRate != sampleRate || highFreq != m_hiFreq) {
        hiDelay = setFrequencyCornersForIntDelay( m_low2, m_size, highFreq / sampleRate, kMaxDelay);
    }
    m_groupDelay = loDelay * 2;
    if(m_loDelay != loDelay)
        m_delay3->setDelay(loDelay * 2);
    if(m_loDelay != loDelay || m_hiDelay != hiDelay)
        m_delay2->setDelay((loDelay - hiDelay ) * 2);
    m_loDelay       = loDelay;
    m_hiDelay       = hiDelay;
    m_loFreq        = lowFreq;
    m_hiFreq        = highFreq;
    m_oldSampleRate = sampleRate;
}
void LVMixEQEffectGroupState::processChannel(const CSAMPLE* pInput, CSAMPLE* pOutput,
                    const int numSamples,
                    const unsigned int sampleRate,
                    double fLow, double fMid, double fHigh,
                    double loFreq, double hiFreq) {

    setFilters(sampleRate, loFreq, hiFreq);

    // Since a Bessel Low pass Filter has a constant group delay in the pass band,
    // we can subtract or add the filtered signal to the dry signal if we compensate this delay
    // The dry signal represents the high gain
    // Then the higher low pass is added and at least the lower low pass result.
    fLow = fLow - fMid;
    fMid = fMid - fHigh;

    // Note: We do not call pauseFilter() here because this will introduce a
    // buffer size-dependent start delay. During such start delay some unwanted
    // frequencies are slipping though or wanted frequencies are damped.
    // We know the exact group delay here so we can just hold off the ramping.
    m_delay3->process(pInput, &m_pHighBuf[0],         numSamples);
    m_delay2->process(pInput, &m_pBandBuf[0],         numSamples);
    m_low2->process  (&m_pBandBuf[0], &m_pBandBuf[0], numSamples);
    m_low1->process  (pInput, &m_pLowBuf[0],          numSamples);

    // Test code for comparing streams as two stereo channels
    //for (unsigned int i = 0; i < numSamples; i +=2) {
    //    pOutput[i] = pState->m_pLowBuf[i];
    //    pOutput[i + 1] = pState->m_pBandBuf[i];
    //}
    SampleUtil::copyWithRampingGain(pOutput,&m_pLowBuf[0], m_oldLow,fLow,numSamples);
    SampleUtil::addWithRampingGain (pOutput,&m_pBandBuf[0],m_oldMid,fMid,numSamples);
    SampleUtil::addWithRampingGain (pOutput,&m_pHighBuf[0],m_oldHigh,fHigh,numSamples);
    m_oldLow = fLow;
    m_oldMid = fMid;
    m_oldHigh = fHigh;
}
