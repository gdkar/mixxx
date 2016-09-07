#include "engine/enginebufferscale.h"

#include "util/defs.h"
#include "util/sample.h"
#include "engine/readaheadmanager.h"

EngineBufferScale::EngineBufferScale(ReadAheadManager *raman, QObject *pParent)
        : QObject(pParent)
        , m_audioSignal(
                mixxx::AudioSignal::SampleLayout::Interleaved,
                mixxx::AudioSignal::kChannelCountStereo,
                mixxx::AudioSignal::kSamplingRateCD)
        , m_pReadAheadManager(raman)
        , m_dBaseRate(1.0)
        , m_bSpeedAffectsPitch(false)
        , m_dTempoRatio(1.0)
        , m_dPitchRatio(1.0)
{
    DEBUG_ASSERT(m_audioSignal.verifyReadable());
}

EngineBufferScale::~EngineBufferScale() = default;

void EngineBufferScale::setSampleRate(SINT iSampleRate)
{
    m_audioSignal = mixxx::AudioSignal(
            m_audioSignal.getSampleLayout(),
            m_audioSignal.getChannelCount(),
            iSampleRate);
    DEBUG_ASSERT(m_audioSignal.verifyReadable());
}
