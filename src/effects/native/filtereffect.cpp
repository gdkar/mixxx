#include "effects/native/filtereffect.h"
#include "util/math.h"

static const double kMinCorner = 0.0003; // 13 Hz @ 44100
static const double kMaxCorner = 0.5; // 22050 Hz @ 44100

// static
QString FilterEffect::getId() {
    return "org.mixxx.effects.filter";
}

// static
EffectManifest FilterEffect::getManifest() {
    EffectManifest manifest;
    manifest.setId(getId());
    manifest.setName(QObject::tr("Filter"));
    manifest.setAuthor("The Mixxx Team");
    manifest.setVersion("1.0");
    manifest.setDescription(QObject::tr("The filter changes the tone of the "
                                        "music by allowing only high or low "
                                        "frequencies to pass through."));
    manifest.setEffectRampsFromDry(true);
    manifest.setIsForFilterKnob(true);

    EffectManifestParameter* lpf = manifest.addParameter();
    lpf->setId("lpf");
    lpf->setName(QObject::tr("LPF"));
    lpf->setDescription(QObject::tr("Corner frequency ratio of the low pass filter"));
    lpf->setControlHint(EffectManifestParameter::CONTROL_KNOB_LOGARITHMIC);
    lpf->setSemanticHint(EffectManifestParameter::SEMANTIC_UNKNOWN);
    lpf->setUnitsHint(EffectManifestParameter::UNITS_UNKNOWN);
    lpf->setDefaultLinkType(EffectManifestParameter::LINK_LINKED_LEFT);
    lpf->setNeutralPointOnScale(1);
    lpf->setDefault(kMaxCorner);
    lpf->setMinimum(kMinCorner);
    lpf->setMaximum(kMaxCorner);

    EffectManifestParameter* q = manifest.addParameter();
    q->setId("q");
    q->setName(QObject::tr("Q"));
    q->setDescription(QObject::tr("Resonance of the filters, default = Flat top"));
    q->setControlHint(EffectManifestParameter::CONTROL_KNOB_LOGARITHMIC);
    q->setSemanticHint(EffectManifestParameter::SEMANTIC_UNKNOWN);
    q->setUnitsHint(EffectManifestParameter::UNITS_SAMPLERATE);
    q->setDefault(0.707106781); // 0.707106781 = Butterworth
    q->setMinimum(0.4);
    q->setMaximum(4.0);

    EffectManifestParameter* hpf = manifest.addParameter();
    hpf->setId("hpf");
    hpf->setName(QObject::tr("HPF"));
    hpf->setDescription(QObject::tr("Corner frequency ratio of the high pass filter"));
    hpf->setControlHint(EffectManifestParameter::CONTROL_KNOB_LOGARITHMIC);
    hpf->setSemanticHint(EffectManifestParameter::SEMANTIC_UNKNOWN);
    hpf->setUnitsHint(EffectManifestParameter::UNITS_UNKNOWN);
    hpf->setDefaultLinkType(EffectManifestParameter::LINK_LINKED_RIGHT);
    hpf->setNeutralPointOnScale(0.0);
    hpf->setDefault(kMinCorner);
    hpf->setMinimum(kMinCorner);
    hpf->setMaximum(kMaxCorner);

    return manifest;
}

FilterGroupState::FilterGroupState()
        : m_loFreq(kMaxCorner),
          m_q(0.707106781),
          m_hiFreq(kMinCorner) {
    m_pBuf = SampleUtil::alloc(MAX_BUFFER_LEN);
    m_pLowFilter = std::make_unique<EngineFilterIIR>(2, EngineFilterIIR::LowPass, QString{"LpBq/%1"}.arg(m_q),
            QString{"LpBq/%1"});
    m_pLowFilter->setFrequencyCorners(1., m_loFreq);
    m_pLowFilter->setStartFromDry(true);
    m_pHighFilter = std::make_unique<EngineFilterIIR>(2, EngineFilterIIR::HighPass, QString{"HpBq/%1"}.arg(m_q),
            QString{"HpBq/%1"});
    m_pHighFilter->setFrequencyCorners(1., m_hiFreq);
    m_pHighFilter->setStartFromDry(true);
}
FilterGroupState::~FilterGroupState()
{
    SampleUtil::free(m_pBuf);
}

FilterEffect::FilterEffect(EngineEffect* pEffect,
                           const EffectManifest& manifest)
        : m_pLPF(pEffect->getParameterById("lpf")),
          m_pQ(pEffect->getParameterById("q")),
          m_pHPF(pEffect->getParameterById("hpf"))
{
    Q_UNUSED(manifest);
}
FilterEffect::~FilterEffect() = default;
void FilterEffect::processChannel(const ChannelHandle& handle,
                                  FilterGroupState* pState,
                                  const CSAMPLE* pInput, CSAMPLE* pOutput,
                                  const unsigned int numSamples,
                                  const unsigned int sampleRate,
                                  const EffectProcessor::EnableState enableState,
                                  const GroupFeatureState& groupFeatures) {
    Q_UNUSED(handle);
    Q_UNUSED(groupFeatures);
    Q_UNUSED(sampleRate);
    auto hpf = 1.;
    auto lpf = 1.;
    auto q = m_pQ->value();

    if (enableState == EffectProcessor::DISABLING) {
        // Ramp to dry, when disabling, this will ramp from dry when enabling as well
        hpf = kMinCorner;
        lpf = kMaxCorner;
    } else {
        hpf = m_pHPF->value();
        lpf = m_pLPF->value();
    }
    if ((pState->m_loFreq != lpf) || (pState->m_q != q) || (pState->m_hiFreq != hpf)) {
        // limit Q to ~4 in case of overlap
        // Determined empirically at 1000 Hz
        auto ratio = hpf / lpf;
        auto clampedQ = q;
        if (ratio < std::sqrt(2.) && ratio >= 1) {
            ratio -= 1;
            auto qmax = 2 + ratio * ratio * ratio * 29;
            clampedQ = math_min(q, qmax);
        } else if (ratio < 1 && ratio >= 0.7) {
            clampedQ = math_min(q, 2.0);
        } else if (ratio < 0.7 && ratio > 0.1) {
            ratio -= 0.1;
            auto qmax = 4 - 2 / 0.6 * ratio;
            clampedQ = math_min(q, qmax);
        }
        pState->m_pLowFilter->setSpec(pState->m_pLowFilter->getTemplate().arg(clampedQ));
        pState->m_pLowFilter->setFrequencyCorners(1, lpf, clampedQ);
        pState->m_pHighFilter->setSpec(pState->m_pHighFilter->getTemplate().arg(clampedQ));
        pState->m_pHighFilter->setFrequencyCorners(1, hpf, clampedQ);
    }
    const auto *pLpfInput = pState->m_pBuf;
    auto pHpfOutput = pState->m_pBuf;
    if (lpf >= kMaxCorner && pState->m_loFreq >= kMaxCorner) {
        // Lpf disabled Hpf can write directly to output
        pHpfOutput = pOutput;
        pLpfInput = pHpfOutput;
    }
    if (hpf > kMinCorner) {
        // hpf enabled, fade-in is handled in the filter when starting from pause
        pState->m_pHighFilter->process(pInput, pHpfOutput, numSamples);
    } else if (pState->m_hiFreq > kMinCorner) {
            // hpf disabling
            pState->m_pHighFilter->processAndPauseFilter(pInput,pHpfOutput, numSamples);
    } else {
        // paused LP uses input directly
        pLpfInput = pInput;
    }
    if (lpf < kMaxCorner) {
        // lpf enabled, fade-in is handled in the filter when starting from pause
        pState->m_pLowFilter->process(pLpfInput, pOutput, numSamples);
    } else if (pState->m_loFreq < kMaxCorner) {
        // hpf disabling
        pState->m_pLowFilter->processAndPauseFilter(pLpfInput,pOutput, numSamples);
    } else if (pLpfInput == pInput) {
        // Both disabled
        if (pOutput != pInput) {
            // We need to copy pInput pOutput
            SampleUtil::copy(pOutput, pInput, numSamples);
        }
    }
    pState->m_loFreq = lpf;
    pState->m_q = q;
    pState->m_hiFreq = hpf;
}
