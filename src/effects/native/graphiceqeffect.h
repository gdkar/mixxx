_Pragma("once")
#include <QMap>

#include "effects/effect.h"
#include "effects/effectprocessor.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/enginefilterbiquad1.h"
#include "util.h"
#include "util/types.h"
#include "util/defs.h"
#include "sampleutil.h"
class GraphicEQEffectGroupState {
  public:
    GraphicEQEffectGroupState();
    virtual ~GraphicEQEffectGroupState();
    void setFilters(int sampleRate);
    EngineFilterBiquad1LowShelving* m_low;
    QList<EngineFilterBiquad1Peaking*> m_bands;
    EngineFilterBiquad1HighShelving* m_high;
    QList<CSAMPLE*> m_pBufs;
    QList<double> m_oldMid;
    double m_oldLow;
    double m_oldHigh;
    float m_centerFrequencies[8];
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
    EngineEffectParameter* m_pPotLow = nullptr;
    QList<EngineEffectParameter*> m_pPotMid;
    EngineEffectParameter* m_pPotHigh = nullptr;
    unsigned int m_oldSampleRate = 0;
};
