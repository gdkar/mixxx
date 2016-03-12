_Pragma("once")
#include <QMap>

#include "controlobjectslave.h"
#include "effects/effect.h"
#include "effects/effectprocessor.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/enginefilteriir.h"
#include "util/class.h"
#include "util/defs.h"
#include "util/sample.h"
#include "util/types.h"

#include <memory>
#include <utility>
#include <vector>

class GraphicEQEffectGroupState {
  public:
    GraphicEQEffectGroupState();
    virtual ~GraphicEQEffectGroupState();
    void setFilters(int sampleRate, double mid[8]);
    std::unique_ptr<EngineFilterIIR>    m_bands[8];
    double m_oldGain[8] = { 0. };
    int    m_oldSampleRate{44100};
    double m_centerFrequencies[8] = { 0.};
};
class GraphicEQEffect : public PerChannelEffectProcessor<GraphicEQEffectGroupState> {
  public:
    GraphicEQEffect(EngineEffect* pEffect, const EffectManifest& manifest);
    virtual ~GraphicEQEffect();
    static QString getId();
    static EffectManifest getManifest();
    // See effectprocessor.h
    void processChannel(const ChannelHandle& handle,
                        GraphicEQEffectGroupState* pState,
                        const CSAMPLE* pInput, CSAMPLE *pOutput,
                        const unsigned int numSamples,
                        const unsigned int sampleRate,
                        const EffectProcessor::EnableState enableState,
                        const GroupFeatureState& groupFeatureState);

  private:
    QString debugString() const;
    EngineEffectParameter* m_pPotGain[8];
    unsigned int m_oldSampleRate{44100};
    DISALLOW_COPY_AND_ASSIGN(GraphicEQEffect);
};
