_Pragma("once")
#include "lvmixeqbase.h"

#include <QMap>


#include "effects/effect.h"
#include "effects/effectprocessor.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/enginefilterbessel8.h"
#include "engine/enginefilterdelay.h"
#include "util.h"
#include "util/types.h"
#include "util/defs.h"
#include "sampleutil.h"
class ControlObject;

class Bessel8LVMixEQEffectGroupState :
        public LVMixEQEffectGroupState<EngineFilterBessel8Low> {
};

class Bessel8LVMixEQEffect : public PerChannelEffectProcessor<Bessel8LVMixEQEffectGroupState> {
  public:
    Bessel8LVMixEQEffect(EngineEffect* pEffect, const EffectManifest& manifest);
    virtual ~Bessel8LVMixEQEffect();

    static QString getId();
    static EffectManifest getManifest();

    // See effectprocessor.h
    void processChannel(const ChannelHandle& handle,
                        Bessel8LVMixEQEffectGroupState* pState,
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
