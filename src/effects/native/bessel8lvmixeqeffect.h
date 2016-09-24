_Pragma("once")
#include <QMap>

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "effects/effect.h"
#include "effects/effectprocessor.h"
#include "effects/native/lvmixeqbase.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "util/class.h"
#include "util/types.h"
#include "util/defs.h"

class Bessel8LVMixEQEffectGroupState :
        public LVMixEQEffectGroupState
{
    public:
        Bessel8LVMixEQEffectGroupState()
            : LVMixEQEffectGroupState(8)
        { }
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

    ControlProxy * m_pLoFreqCorner;
    ControlProxy * m_pHiFreqCorner;

};
