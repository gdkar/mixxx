_Pragma("once")
#include <QMap>

#include "util.h"
#include "util/defs.h"
#include "util/types.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "effects/effectprocessor.h"
#include "sampleutil.h"

struct FlangerGroupState {
    FlangerGroupState()
            : delayPos(0),
              time(0) {
        SampleUtil::applyGain(delayLeft, 0, MAX_BUFFER_LEN);
        SampleUtil::applyGain(delayRight, 0, MAX_BUFFER_LEN);
    }
    CSAMPLE delayRight[MAX_BUFFER_LEN];
    CSAMPLE delayLeft[MAX_BUFFER_LEN];
    unsigned int delayPos;
    unsigned int time;
};

class FlangerEffect : public PerChannelEffectProcessor<FlangerGroupState> {
  public:
    FlangerEffect(EngineEffect* pEffect, const EffectManifest& manifest);
    virtual ~FlangerEffect();

    static QString getId();
    static EffectManifest getManifest();

    // See effectprocessor.h
    void processChannel(const ChannelHandle& handle,
                        FlangerGroupState* pState,
                        const CSAMPLE* pInput, CSAMPLE* pOutput,
                        const unsigned int numSamples,
                        const unsigned int sampleRate,
                        const EffectProcessor::EnableState enableState,
                        const GroupFeatureState& groupFeatures);
  private:
    QString debugString() const;
    EngineEffectParameter* m_pPeriodParameter;
    EngineEffectParameter* m_pDepthParameter;
    EngineEffectParameter* m_pDelayParameter;
};
