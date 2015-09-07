#ifndef LINKWITZRILEYEQEFFECT_H
#define LINKWITZRILEYEQEFFECT_H

#include <QMap>

#include "effects/effect.h"
#include "effects/effectprocessor.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/enginefilterlinkwitzriley8.h"
#include "util.h"
#include "util/types.h"
#include "util/defs.h"
#include "sampleutil.h"

class ControlObjectSlave;
class LinkwitzRiley8EQEffectGroupState {
  public:
    LinkwitzRiley8EQEffectGroupState();
    virtual ~LinkwitzRiley8EQEffectGroupState();
    void setFilters(int sampleRate, int lowFreq, int highFreq);
    EngineFilterLinkwtzRiley8Low* m_low1 = nullptr;
    EngineFilterLinkwtzRiley8High* m_high1 = nullptr;
    EngineFilterLinkwtzRiley8Low* m_low2 = nullptr;
    EngineFilterLinkwtzRiley8High* m_high2 = nullptr;
    double old_low = 0;
    double old_mid = 0;
    double old_high= 0;

    CSAMPLE* m_pLowBuf = nullptr;
    CSAMPLE* m_pBandBuf = nullptr;
    CSAMPLE* m_pHighBuf = nullptr;
    unsigned int m_oldSampleRate;
    int m_loFreq = 0;
    int m_hiFreq = 0;
};
class LinkwitzRiley8EQEffect : public PerChannelEffectProcessor<LinkwitzRiley8EQEffectGroupState> {
  public:
    LinkwitzRiley8EQEffect(EngineEffect* pEffect, const EffectManifest& manifest);
    virtual ~LinkwitzRiley8EQEffect();
    static QString getId();
    static EffectManifest getManifest();
    // See effectprocessor.h
    void processChannel(const ChannelHandle& handle,
                        LinkwitzRiley8EQEffectGroupState* pState,
                        const CSAMPLE* pInput, CSAMPLE *pOutput,
                        const unsigned int numSamples,
                        const unsigned int sampleRate,
                        const EffectProcessor::EnableState enableState,
                        const GroupFeatureState& groupFeatureState);

  private:
    QString debugString() const {return getId();}
    EngineEffectParameter* m_pPotLow = nullptr;
    EngineEffectParameter* m_pPotMid = nullptr;
    EngineEffectParameter* m_pPotHigh= nullptr;

    EngineEffectParameter* m_pKillLow = nullptr;
    EngineEffectParameter* m_pKillMid = nullptr;
    EngineEffectParameter* m_pKillHigh= nullptr;

    ControlObjectSlave* m_pLoFreqCorner = nullptr;
    ControlObjectSlave* m_pHiFreqCorner = nullptr;
    DISALLOW_COPY_AND_ASSIGN(LinkwitzRiley8EQEffect);
};

#endif /* LINKWITZRILEYEQEFFECT_H */
