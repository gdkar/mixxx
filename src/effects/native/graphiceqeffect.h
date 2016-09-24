_Pragma("once")
#include <QMap>

#include "control/controlproxy.h"
#include "control/controlobject.h"
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
    size_t m_size{8};
    void setFilters(int sampleRate, std::vector<double>& );
    std::vector<std::unique_ptr<EngineFilterIIR> > m_bands{8};
    std::vector<double> m_oldGain{8};
    std::vector<double> m_centerFrequencies{8};
    unsigned int m_oldSampleRate{44100};
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
    std::vector<EngineEffectParameter*> m_pPotGain{8};
    unsigned int m_oldSampleRate{44100};
};
