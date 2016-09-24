_Pragma("once")
#include "engine/enginefilterdelay.h"
#include "engine/enginefilteriir.h"
#include "util/defs.h"
#include "util/math.h"
#include "util/sample.h"
#include "util/types.h"

class LVMixEQEffectGroupState {
  protected:
    size_t               m_size{4};
    std::unique_ptr<EngineFilterIIR> m_low1;
    std::unique_ptr<EngineFilterIIR> m_low2;
    std::unique_ptr<EngineFilterDelay> m_delay2;
    std::unique_ptr<EngineFilterDelay> m_delay3;

    double m_oldLow;
    double m_oldMid;
    double m_oldHigh;

    int m_groupDelay;

    int m_oldSampleRate;
    double m_loFreq;
    double m_hiFreq;
    int    m_loDelay;
    int    m_hiDelay;
    std::unique_ptr<CSAMPLE[]> m_pLowBuf;
    std::unique_ptr<CSAMPLE[]> m_pBandBuf;
    std::unique_ptr<CSAMPLE[]> m_pHighBuf;

  public:
    LVMixEQEffectGroupState(size_t size);
    virtual ~LVMixEQEffectGroupState();
    virtual void setFilters(int sampleRate, double lowFreq, double highFreq);
    virtual void processChannel(const CSAMPLE* pInput, CSAMPLE* pOutput,
                        const int numSamples,
                        const unsigned int sampleRate,
                        double fLow, double fMid, double fHigh,
                        double loFreq, double hiFreq);
};
