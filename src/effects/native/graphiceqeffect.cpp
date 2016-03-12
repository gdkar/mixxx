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
    manifest.setDescription(QObject::tr(
        "An 8 band Graphic EQ based on Biquad Filters"));
    manifest.setEffectRampsFromDry(true);
    manifest.setIsMasterEQ(true);

    // Display rounded center frequencies for each filter
    float centerFrequencies[8] = {45, 100, 220, 500, 1100, 2500,5500, 12000};
    for (auto i = 0; i < 8; i++) {
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
        band->setMinimum(-24);
        band->setMaximum(24);
    }
    return manifest;
}

GraphicEQEffectGroupState::GraphicEQEffectGroupState()
{
    // Initialize the default center frequencies
    m_centerFrequencies[0] = 81.;
    m_centerFrequencies[1] = 100.;
    m_centerFrequencies[2] = 222.;
    m_centerFrequencies[3] = 494.;
    m_centerFrequencies[4] = 1097.;
    m_centerFrequencies[5] = 2437.;
    m_centerFrequencies[6] = 5416.;
    m_centerFrequencies[7] = 9828.;

    // Initialize the filters with default parameters
    m_bands[0] = std::make_unique<EngineFilterIIR>(5,IIR_LP,  QString{"LsBq/%1/%2"}.arg(Q).arg(0));//44100, m_centerFrequencies[0], Q);
    m_bands[0]->setCoefs(44100., 5, QString{"LsBq/%1/%2"}.arg(Q).arg(0),m_centerFrequencies[0]);
    for (int i = 1; i < 8; i++) {
        m_bands[i] = std::make_unique<EngineFilterIIR>(5,IIR_BP, QString{"PkBq/%1/%2"}.arg(Q).arg(0));
        m_bands[i]->setCoefs(44100., 5, QString{"PkBq/%1/%2"}.arg(Q).arg(0),m_centerFrequencies[i]);
    }
    m_bands[7] = std::make_unique<EngineFilterIIR>(5,IIR_HP,  QString{"HsBq/%1/%2"}.arg(Q).arg(0));
    m_bands[7]->setCoefs(44100., 5, QString{"HsBq/%1/%2"}.arg(Q).arg(0),m_centerFrequencies[7]);
}
GraphicEQEffectGroupState::~GraphicEQEffectGroupState() = default;
void GraphicEQEffectGroupState::setFilters(int sampleRate, double gain[8])
{
    if(m_oldSampleRate != sampleRate || m_oldGain[0] != gain[0])
        m_bands[0]->setCoefs(sampleRate, 5, QString{"LsBq/%1/%2"}.arg(Q).arg(gain[0]),m_centerFrequencies[0]);
    m_oldGain[0] = gain[0];
    for (auto i = 1; i < 8; i++) {
        if(sampleRate != m_oldSampleRate || gain[i] != m_oldGain[i])
            m_bands[i]->setCoefs(sampleRate, 5, QString{"PkBq/%1/%2"}.arg(Q).arg(gain[i]),m_centerFrequencies[i]);
        m_oldGain [i] = gain[i+1];
    }
    if(m_oldSampleRate != sampleRate || m_oldGain[7] != gain[7])
        m_bands[7]->setCoefs(sampleRate, 5, QString{"HsBq/%1/%2"}.arg(Q).arg(gain[7]),m_centerFrequencies[7]);
    m_oldGain[7] = gain[7];
    m_oldSampleRate = sampleRate;
}
GraphicEQEffect::GraphicEQEffect(EngineEffect* pEffect,const EffectManifest& manifest)
        : m_oldSampleRate(44100)
{
    Q_UNUSED(manifest);
    for (auto i = 0; i < 8; i++)
        m_pPotGain[i] = pEffect->getParameterById(QString("band%1").arg(i));
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
    double fGain[8] = { 0.};
    if (enableState == EffectProcessor::DISABLING) {
         // Ramp to dry, when disabling, this will ramp from dry when enabling as well
        std::fill_n(fGain,8,1.);
    } else {
        for(auto i = 0; i < 8; i++)
            fGain[i] = m_pPotGain[i]->value();
    }
    pState->setFilters(sampleRate, fGain);
    pState->m_bands[0]->process(pIn,pOut,numSamples);
    for (auto i = 1; i < 6; i++)
        pState->m_bands[i]->process(pOut,pOut, numSamples);
    if (enableState == EffectProcessor::DISABLING) {
        for (auto i = 0; i < 8; i++)
            pState->m_bands[i]->pauseFilter();
    }
}
