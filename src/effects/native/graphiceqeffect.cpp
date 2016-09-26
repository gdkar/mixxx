#include "effects/native/graphiceqeffect.h"
#include "util/math.h"

#define Q 1.2247449

// static
QString GraphicEQEffect::debugString() const
{
    return getId();
}
QString GraphicEQEffect::getId() {
    return "org.mixxx.effects.graphiceq";
}

// static
EffectManifest GraphicEQEffect::getManifest() {
    EffectManifest manifest;
    manifest.setId(getId());
    manifest.setName(QObject::tr("Graphic EQ"));
    manifest.setAuthor("The Mixxx Team");
    manifest.setVersion("1.0");
    manifest.setDescription(QObject::tr("An 8 band Graphic EQ based on Biquad Filters"));
    manifest.setIsMasterEQ(true);

    // Display rounded center frequencies for each filter
    float centerFrequencies[] = {45, 100, 220, 500, 1100, 2500,5500, 12000};
    for(auto i =  0u; i < sizeof(centerFrequencies)/sizeof(centerFrequencies[0]);i++) {
        auto paramName = QString{};
        if (centerFrequencies[i ] < 1000) {
            paramName = QString("%1 Hz").arg(centerFrequencies[i ]);
        } else {
            paramName = QString("%1 kHz").arg(centerFrequencies[i ] / 1000);
        }
        auto band = manifest.addParameter();
        band->setId(QString("band%1").arg(i));
        band->setName(paramName);
        band->setDescription(QObject::tr("Gain for Band Filter %1").arg(i));
        band->setControlHint(EffectManifestParameter::CONTROL_KNOB_LINEAR);
        band->setSemanticHint(EffectManifestParameter::SEMANTIC_UNKNOWN);
        band->setUnitsHint(EffectManifestParameter::UNITS_UNKNOWN);
        band->setNeutralPointOnScale(0.5);
        band->setDefault(0);
        band->setMinimum(-12);
        band->setMaximum(12);
    }
    return manifest;
}

GraphicEQEffectGroupState::GraphicEQEffectGroupState()
{
    // Initialize the default center frequencies
    m_centerFrequencies = std::vector<double>{45, 100, 220, 500, 1100, 2500,5500, 12000};
    m_size = m_centerFrequencies.size();
    m_oldGain.resize(m_size);
    m_bands.resize(m_size);

    // Initialize the filters with default parameters
    m_bands[0] = std::make_unique<EngineFilterIIR>(5,IIR_LP,  QString{"LsBq/%1/%2"}.arg(Q).arg(0));//44100, m_centerFrequencies[0], Q);
    m_bands[0]->setStartFromDry(true);
    m_bands[0]->setCoefs(44100., 5, QString{"LsBq/%1/%2"}.arg(Q).arg(0),m_centerFrequencies[0]);
    auto i = 1u;
    for (; i < m_size - 1; i++) {
        m_bands[i] = std::make_unique<EngineFilterIIR>(5,IIR_BP, QString{"PkBq/%1/%2"}.arg(Q).arg(0));
        m_bands[i]->setStartFromDry(true);
        m_bands[i]->setCoefs(44100., 5, QString{"PkBq/%1/%2"}.arg(Q).arg(0),m_centerFrequencies[i]);
    }
    m_bands[i] = std::make_unique<EngineFilterIIR>(5,IIR_BP, QString{"HsBq/%1/%2"}.arg(Q).arg(0));
    m_bands[i]->setStartFromDry(true);
    m_bands[i]->setCoefs(44100., 5, QString{"HsBq/%1/%2"}.arg(Q).arg(0),m_centerFrequencies[i]);
}
GraphicEQEffectGroupState::~GraphicEQEffectGroupState() = default;
void GraphicEQEffectGroupState::setFilters(int sampleRate, std::vector<double> &gain)
{
    if((int)m_oldSampleRate != sampleRate || m_oldGain[0] != gain[0])
        m_bands[0]->setCoefs(sampleRate, 5, QString{"LsBq/%1/%2"}.arg(Q).arg(gain[0]),m_centerFrequencies[0]);
    m_oldGain[0] = gain[0];
    auto i = 1u;
    for (; i < m_size - 1; i++) {
        if(sampleRate != (int)m_oldSampleRate || gain[i] != m_oldGain[i])
            m_bands[i]->setCoefs(sampleRate, 5, QString{"PkBq/%1/%2"}.arg(Q).arg(gain[i]),m_centerFrequencies[i]);
        m_oldGain [i] = gain[i];
    }
    if((int)m_oldSampleRate != sampleRate || m_oldGain[i] != gain[i])
        m_bands[i]->setCoefs(sampleRate, 5, QString{"HsBq/%1/%2"}.arg(Q).arg(gain[i]),m_centerFrequencies[i]);
    m_oldGain[i] = gain[i];
    m_oldSampleRate = sampleRate;
}
GraphicEQEffect::GraphicEQEffect(EngineEffect* pEffect,const EffectManifest& manifest)
        : m_oldSampleRate(44100)
{
    Q_UNUSED(manifest);
    m_pPotGain.clear();
    for (auto i = 0u; i < 8u; i++)
        m_pPotGain.push_back(pEffect->getParameterById(QString("band%1").arg(i)));
}
GraphicEQEffect::~GraphicEQEffect() = default;
void GraphicEQEffect::processChannel(const ChannelHandle& handle,
                                     GraphicEQEffectGroupState* pState,
                                     const CSAMPLE* pIn, CSAMPLE* pOut,
                                     const unsigned int numSamples,
                                     const unsigned int sampleRate,
                                     const EffectProcessor::EnableState enableState,
                                     const GroupFeatureState& groupFeatures)
{
    Q_UNUSED(handle);
    Q_UNUSED(groupFeatures);
    // If the sample rate has changed, initialize the filters using the new
    // sample rate
    auto fGain = std::vector<double>(pState->m_size, 0.);
    if (enableState == EffectProcessor::DISABLING) {
         // Ramp to dry, when disabling, this will ramp from dry when enabling as well
        std::fill(fGain.begin(),fGain.end(), 0);
    } else {
        for(auto i = 0u; i < fGain.size() && i < m_pPotGain.size(); i++) {
            fGain[i] = m_pPotGain[i]->value();
        }
    }
    pState->setFilters(sampleRate, fGain);
    auto pFrom = pIn;
    auto pTo   = pOut;
    for (auto i = 0u; i < fGain.size() && i < pState->m_size; i++) {
        if(fGain[i] || pState->m_oldGain[i]) {
            pState->m_bands[i]->process(pFrom,pTo, numSamples);
            pFrom = pTo;
        }
    }
    if(pFrom != pTo)
        SampleUtil::copy(pTo, pFrom, numSamples);

    if (enableState == EffectProcessor::DISABLING) {
        for (auto i = 0u; i < 8u; i++) {
            pState->m_bands[i]->pauseFilter();
            pState->m_oldGain[i] = 0.;
        }
    }
}
