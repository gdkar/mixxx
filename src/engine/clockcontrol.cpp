#include "engine/clockcontrol.h"

#include "preferences/usersettings.h"
#include "engine/enginecontrol.h"
#include "control/controlproxy.h"

ClockControl::ClockControl(QString group, UserSettingsPointer pConfig)
        : EngineControl(group, pConfig) {
    m_pCOBeatActive = new ControlProxy(ConfigKey(group, "beat_active"),this);
    m_pCOBeatActive->set(0.0);
    m_pCOSampleRate = new ControlProxy("[Master]","samplerate",this);
}
ClockControl::~ClockControl() { }
void ClockControl::trackLoaded(TrackPointer pNewTrack, TrackPointer pOldTrack)
{
    Q_UNUSED(pOldTrack);
    // Clear on-beat control
    m_pCOBeatActive->set(0.0);
    // Disconnect any previously loaded track/beats
    if (m_pTrack) {
        disconnect(m_pTrack.data(), nullptr,this, nullptr);
    }
    if (pNewTrack) {
        m_pTrack = pNewTrack;
        connect(m_pTrack.data(), &Track::beatsUpdated, this,
            [pNewTrack,this]() { m_pBeats = pNewTrack->getBeats();});
        m_pBeats = m_pTrack->getBeats();
    } else {
        m_pBeats.clear();
        m_pTrack.clear();
    }

}
double ClockControl::process(double dRate,
                             double currentSample,
                             double totalSamples,
                             int iBuffersize)
{
    Q_UNUSED(totalSamples);
    Q_UNUSED(iBuffersize);
    auto samplerate = m_pCOSampleRate->get();
    // TODO(XXX) should this be customizable, or latency dependent?
    auto blinkSeconds = 0.100;
    // Multiply by two to get samples from frames. Interval is scaled linearly
    // by the rate.
    auto blinkIntervalSamples = 2.0 * samplerate * (1.0 * dRate) * blinkSeconds;
    if (m_pBeats) {
        auto closestBeat = m_pBeats->findClosestBeat(currentSample);
        auto distanceToClosestBeat = fabs(currentSample - closestBeat);
        m_pCOBeatActive->set(distanceToClosestBeat < blinkIntervalSamples / 2.0);
    }
    return kNoTrigger;
}
