_Pragma("once")
#include "effects/effect.h"
#include "effects/effectprocessor.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/enginefilteriir.h"

#include "util/defs.h"
#include "util/sample.h"
#include "util/types.h"
#include <memory>
#include <utility>

struct FilterGroupState {
    FilterGroupState();
    ~FilterGroupState();
    void setFilters(int sampleRate, double lowFreq, double highFreq);
    CSAMPLE* m_pBuf;
    std::unique_ptr<EngineFilterIIR> m_pLowFilter;
    std::unique_ptr<EngineFilterIIR> m_pHighFilter;
    double m_loFreq;
    double m_q;
    double m_hiFreq;
};
class FilterEffect : public PerChannelEffectProcessor<FilterGroupState> {
  public:
    FilterEffect(EngineEffect* pEffect, const EffectManifest& manifest);
    virtual ~FilterEffect();
    static QString getId();
    static EffectManifest getManifest();
    // See effectprocessor.h
    void processChannel(const ChannelHandle& handle,
                        FilterGroupState* pState,
                        const CSAMPLE* pInput, CSAMPLE *pOutput,
                        const unsigned int numSamples,
                        const unsigned int sampleRate,
                        const EffectProcessor::EnableState enableState,
                        const GroupFeatureState& groupFeatures);
  private:
    QString debugString() const {
        return getId();
    }
    EngineEffectParameter* m_pLPF;
    EngineEffectParameter* m_pQ;
    EngineEffectParameter* m_pHPF;
};
