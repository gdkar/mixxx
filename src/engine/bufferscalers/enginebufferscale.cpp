#include "engine/bufferscalers/enginebufferscale.h"

#include "engine/engine.h"

#include "util/defs.h"
#include "util/sample.h"

EngineBufferScale::EngineBufferScale(
    ReadAheadManager *raman, QObject *p)
        : QObject(p)
        , m_audioSignal(
                mixxx::AudioSignal::SampleLayout::Interleaved,
                mixxx::AudioSignal::ChannelCount(mixxx::kEngineChannelCount),
                mixxx::AudioSignal::SampleRate(44100)
        )
        , m_pReadAheadManager(raman),
          m_dBaseRate(1.0),
          m_bSpeedAffectsPitch(false),
          m_dTempoRatio(1.0),
          m_dPitchRatio(1.0)
{
    DEBUG_ASSERT(m_audioSignal.verifyReadable());
}

EngineBufferScale::~EngineBufferScale() = default;

void EngineBufferScale::setSampleRate(SINT iSampleRate)
{
    m_audioSignal = mixxx::AudioSignal(
            m_audioSignal.sampleLayout(),
            m_audioSignal.channelCount(),
            mixxx::AudioSignal::SampleRate(iSampleRate));
    DEBUG_ASSERT(m_audioSignal.verifyReadable());
}
void EngineBufferScale::setScaleParameters(
    double base_rate,
    double* pTempoRatio,
    double* pPitchRatio
    )
{
    m_dBaseRate = base_rate;
    m_dTempoRatio = *pTempoRatio;
    m_dPitchRatio = *pPitchRatio;
}
const mixxx::AudioSignal& EngineBufferScale::getAudioSignal() const
{
    return m_audioSignal;
}

