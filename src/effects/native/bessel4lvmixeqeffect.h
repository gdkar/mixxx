_Pragma("once")
#include <QMap>

#include "effects/effect.h"
#include "effects/effectprocessor.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/enginefilterbessel4.h"
#include "engine/enginefilterdelay.h"
#include "util.h"
#include "util/types.h"
#include "util/defs.h"
#include "lvmixeqbase.h"
class ControlObject;
class Bessel4LVMixEQEffectGroupState :
        public LVMixEQEffectGroupState<EngineFilterBessel4Low> {
};

class Bessel4LVMixEQEffect : public PerChannelEffectProcessor<Bessel4LVMixEQEffectGroupState> {
  public:
    Bessel4LVMixEQEffect(EngineEffect* pEffect, const EffectManifest& manifest);
    virtual ~Bessel4LVMixEQEffect();

    static QString getId();
    static EffectManifest getManifest();

    // See effectprocessor.h
    void processChannel(const ChannelHandle& handle,
                        Bessel4LVMixEQEffectGroupState* pState,
                        const CSAMPLE* pInput, CSAMPLE* pOutput,
                        const unsigned int numSamples,
                        const unsigned int sampleRate,
                        const EffectProcessor::EnableState enableState,
                        const GroupFeatureState& groupFeatureState);

  private:
    QString debugString() const {
        return getId();
    }

    EngineEffectParameter* m_pPotLow;
    EngineEffectParameter* m_pPotMid;
    EngineEffectParameter* m_pPotHigh;

    EngineEffectParameter* m_pKillLow;
    EngineEffectParameter* m_pKillMid;
    EngineEffectParameter* m_pKillHigh;

    ControlObject* m_pLoFreqCorner;
    ControlObject* m_pHiFreqCorner;

};
