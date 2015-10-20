#include <QMessageBox>

#include "player.h"
#include "playerinfo.h"

#include "controlobject.h"
#include "controlpotmeter.h"
#include "trackinfoobject.h"
#include "engine/enginebuffer.h"
#include "engine/enginedeck.h"
#include "engine/enginemaster.h"
#include "track/beatgrid.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "anqueue/analyserqueue.h"
#include "util/sandbox.h"
#include "effects/effectsmanager.h"

Player::Player(QObject* pParent,
                                         ConfigObject<ConfigValue>* pConfig,
                                         EngineMaster* pMixingEngine,
                                         EffectsManager* pEffectsManager,
                                         EngineChannel::ChannelOrientation defaultOrientation,
                                         QString group,
                                         bool defaultMaster,
                                         bool defaultHeadphones)
        : QObject(pParent),
          m_group(group),
          m_pConfig(pConfig),
          m_replaygainPending(false)
{
    auto channelGroup = pMixingEngine->registerChannelGroup(group);
    m_pChannel = new EngineDeck(channelGroup, pConfig, pMixingEngine,pEffectsManager, defaultOrientation);
    auto pEngineBuffer = m_pChannel->getEngineBuffer();
    pMixingEngine->addChannel(m_pChannel);
    // Set the routing option defaults for the master and headphone mixes.
    m_pChannel->setMaster(defaultMaster);
    m_pChannel->setPfl(defaultHeadphones);
    // Connect our signals and slots with the EngineBuffer's signals and
    // slots. This will let us know when the reader is done loading a track, and
    // let us request that the reader load a track.
    connect(this, SIGNAL(loadTrack(TrackPointer, bool)),pEngineBuffer, SLOT(slotLoadTrack(TrackPointer, bool)));
    connect(pEngineBuffer, SIGNAL(trackLoaded(TrackPointer)),this, SLOT(slotFinishLoading(TrackPointer)));
    connect(pEngineBuffer, SIGNAL(trackLoadFailed(TrackPointer, QString)),this, SLOT(slotLoadFailed(TrackPointer, QString)));
    connect(pEngineBuffer, SIGNAL(trackUnloaded(TrackPointer)),this, SLOT(slotUnloadTrack(TrackPointer)));
    // Get loop point control objects
    m_pLoopInPoint = new ControlObject(ConfigKey(getGroup(),"loop_start_position"),this);
    m_pLoopOutPoint = new ControlObject(ConfigKey(getGroup(),"loop_end_position"),this);
    // Duration of the current song, we create this one because nothing else does.
    m_pDuration = new ControlObject(ConfigKey(getGroup(), "duration"),this);
    // Waveform controls
    m_pWaveformZoom = new ControlPotmeter(ConfigKey(group, "waveform_zoom"),WaveformWidgetRenderer::s_waveformMinZoom,WaveformWidgetRenderer::s_waveformMaxZoom);
    m_pWaveformZoom->setParent(this);
    m_pWaveformZoom->set(1.0);
    m_pWaveformZoom->setProperty("stepCount",WaveformWidgetRenderer::s_waveformMaxZoom -WaveformWidgetRenderer::s_waveformMinZoom);
    m_pWaveformZoom->setProperty("smallStepCount",WaveformWidgetRenderer::s_waveformMaxZoom -WaveformWidgetRenderer::s_waveformMinZoom);
    m_pEndOfTrack = new ControlObject(ConfigKey(group, "end_of_track"));
    m_pEndOfTrack->set(0.);
    m_pEndOfTrack->setParent(this);
    m_pPreGain = new ControlObject(ConfigKey(group, "pregain"),this);
    //BPM of the current song
    m_pBPM = new ControlObject(ConfigKey(group, "file_bpm"),this);
    m_pKey = new ControlObject(ConfigKey(group, "file_key"),this);
    m_pReplayGain = new ControlObject(ConfigKey(group, "replaygain"),this);
    m_pPlay = new ControlObject(ConfigKey(group, "play"),this);
    connect(m_pPlay, SIGNAL(valueChanged(double)),this, SLOT(slotPlayToggled(double)));
}
QString Player::getGroup()const
{
  return m_group;
}
Player::~Player()
{
    if (m_pLoadedTrack)
    {
        emit(unloadingTrack(m_pLoadedTrack));
        disconnect(m_pLoadedTrack.data(), 0, m_pBPM, 0);
        disconnect(m_pLoadedTrack.data(), 0, this, 0);
        disconnect(m_pLoadedTrack.data(), 0, m_pKey, 0);
        m_pLoadedTrack.clear();
    }
}

void Player::slotLoadTrack(TrackPointer track, bool bPlay)
{
    // Before loading the track, ensure we have access. This uses lazy
    // evaluation to make sure track isn't nullptr before we dereference it.
    if (!track.isNull() && !Sandbox::askForAccess(track->getCanonicalLocation())) return;
    //Disconnect the old track's signals.
    if (m_pLoadedTrack)
    {
        // Save the loops that are currently set in a loop cue. If no loop cue is
        // currently on the track, then create a new one.
        int loopStart = m_pLoopInPoint->get();
        int loopEnd = m_pLoopOutPoint->get();
        if (loopStart != -1 && loopEnd != -1 &&
            even(loopStart) && even(loopEnd) && loopStart <= loopEnd)
        {
            Cue* pLoopCue = nullptr;
            auto cuePoints = m_pLoadedTrack->getCuePoints();
            QListIterator<Cue*> it(cuePoints);
            while (it.hasNext())
            {
                Cue* pCue = it.next();
                if (pCue->getType() == Cue::LOOP) pLoopCue = pCue;
            }
            if (!pLoopCue)
            {
                pLoopCue = m_pLoadedTrack->addCue();
                pLoopCue->setType(Cue::LOOP);
            }
            pLoopCue->setPosition(loopStart);
            pLoopCue->setLength(loopEnd - loopStart);
        }
        // WARNING: Never. Ever. call bare disconnect() on an object. Mixxx
        // relies on signals and slots to get tons of things done. Don't
        // randomly disconnect things.
        // m_pLoadedTrack->disconnect();
        disconnect(m_pLoadedTrack.data(), 0, m_pBPM, 0);
        disconnect(m_pLoadedTrack.data(), 0, this, 0);
        disconnect(m_pLoadedTrack.data(), 0, m_pKey, 0);
        m_pReplayGain->set(0);
        // Causes the track's data to be saved back to the library database.
        emit(unloadingTrack(m_pLoadedTrack));
    }
    m_pLoadedTrack = track;
    if (m_pLoadedTrack)
    {
        // Listen for updates to the file's BPM
        connect(m_pLoadedTrack.data(), &TrackInfoObject::bpmUpdated,m_pBPM, static_cast<void (ControlObject::*)(double)>((&ControlObject::set)));
        connect(m_pLoadedTrack.data(), &TrackInfoObject::keyUpdated,m_pKey, static_cast<void (ControlObject::*)(double)>((&ControlObject::set)));
        // Listen for updates to the file's Replay Gain
        connect(m_pLoadedTrack.data(), &TrackInfoObject::replayGainUpdated,this, &Player::slotSetReplayGain);
    }
    // Request a new track from the reader
    emit(loadTrack(track, bPlay));
}
void Player::slotLoadFailed(TrackPointer track, QString reason)
{
    // This slot can be delayed until a new  track is already loaded
    // We must not unload the track here
    if (track)
    {
        qDebug() << "Failed to load track" << track->getLocation() << reason;
        emit(loadTrackFailed(track));
    }
    else  qDebug() << "Failed to load track (nullptr track object)" << reason;
    // Alert user.
    QMessageBox::warning(nullptr, tr("Couldn't load track."), reason);
}
void Player::slotUnloadTrack(TrackPointer)
{
    if (m_pLoadedTrack)
    {
        // WARNING: Never. Ever. call bare disconnect() on an object. Mixxx
        // relies on signals and slots to get tons of things done. Don't
        // randomly disconnect things.
        // m_pLoadedTrack->disconnect();
        disconnect(m_pLoadedTrack.data(), 0, m_pBPM, 0);
        disconnect(m_pLoadedTrack.data(), 0, this, 0);
        disconnect(m_pLoadedTrack.data(), 0, m_pKey, 0);

        // Causes the track's data to be saved back to the library database and
        // for all the widgets to unload the track and blank themselves.
        emit(unloadingTrack(m_pLoadedTrack));
    }
    m_replaygainPending = false;
    m_pDuration->set(0);
    m_pBPM->set(0);
    m_pKey->set(0);
    m_pReplayGain->set(0);
    m_pLoopInPoint->set(-1);
    m_pLoopOutPoint->set(-1);
    m_pLoadedTrack.clear();

    // Update the PlayerInfo class that is used in EngineShoutcast to replace
    // the metadata of a stream
    PlayerInfo::instance().setTrackInfo(getGroup(), m_pLoadedTrack);
}
void Player::slotFinishLoading(TrackPointer pTrackInfoObject)
{
    m_replaygainPending = false;
    // Read the tags if required
    if (!m_pLoadedTrack->getHeaderParsed()) m_pLoadedTrack->parse(false);
    // m_pLoadedTrack->setPlayedAndUpdatePlaycount(true); // Actually the song is loaded but not played
    // Update the BPM and duration values that are stored in ControlObjects
    m_pDuration->set(m_pLoadedTrack->getDuration());
    m_pBPM->set(m_pLoadedTrack->getBpm());
    m_pKey->set(m_pLoadedTrack->getKey());
    m_pReplayGain->set(m_pLoadedTrack->getReplayGain());
    // Update the PlayerInfo class that is used in EngineShoutcast to replace
    // the metadata of a stream
    PlayerInfo::instance().setTrackInfo(getGroup(), m_pLoadedTrack);
    // Reset the loop points.
    m_pLoopInPoint->set(-1);
    m_pLoopOutPoint->set(-1);
    const QList<Cue*> trackCues = pTrackInfoObject->getCuePoints();
    QListIterator<Cue*> it(trackCues);
    while (it.hasNext())
    {
        Cue* pCue = it.next();
        if (pCue->getType() == Cue::LOOP)
        {
            int loopStart = pCue->getPosition();
            int loopEnd = loopStart + pCue->getLength();
            if (loopStart != -1 && loopEnd != -1 && even(loopStart) && even(loopEnd))
            {
                m_pLoopInPoint->set(loopStart);
                m_pLoopOutPoint->set(loopEnd);
                break;
            }
        }
    }
    if(m_pConfig->getValueString(ConfigKey("Mixer Profile", "EqAutoReset"), 0).toInt())
    {
        if (m_pLowFilter) m_pLowFilter->set(1.0);
        if (m_pMidFilter) m_pMidFilter->set(1.0);
        if (m_pHighFilter) m_pHighFilter->set(1.0);
        if (m_pLowFilterKill) m_pLowFilterKill->set(0.0);
        if (m_pMidFilterKill) m_pMidFilterKill->set(0.0);
        if (m_pHighFilterKill) m_pHighFilterKill->set(0.0);
        m_pPreGain->set(1.0);
    }
    int reset = m_pConfig->getValueString(ConfigKey(
            "Controls", "SpeedAutoReset"),
            QString("%1").arg(RESET_PITCH)).toInt();
    switch (reset)
    {
      case RESET_PITCH_AND_SPEED:
        // Note: speed may affect pitch
        if (m_pSpeed) m_pSpeed->set(0.0);
        // Fallthrough intended
      case RESET_PITCH:
        if (m_pPitchAdjust) m_pPitchAdjust->set(0.0);
    }
    emit(newTrackLoaded(m_pLoadedTrack));
}
TrackPointer Player::getLoadedTrack() const
{
  return m_pLoadedTrack;
}
void Player::slotSetReplayGain(double replayGain)
{
    // Do not change replay gain when track is playing because
    // this may lead to an unexpected volume change
    if (!(*m_pPlay) ) m_pReplayGain->set(replayGain);
    else m_replaygainPending = true;
}

void Player::slotPlayToggled(double v)
{
    if (!v && m_replaygainPending)
    {
        m_pReplayGain->set(m_pLoadedTrack->getReplayGain());
        m_replaygainPending = false;
    }
}
EngineDeck* Player::getEngineDeck() const
{ 
  return m_pChannel;
}
void Player::setupEqControls()
{
    auto group = getGroup();
    m_pLowFilter = new ControlObject(ConfigKey(group, "filterLow"),this);
    m_pMidFilter = new ControlObject(ConfigKey(group, "filterMid"),this);
    m_pHighFilter = new ControlObject(ConfigKey(group, "filterHigh"),this);
    m_pLowFilterKill = new ControlObject(ConfigKey(group, "filterLowKill"),this);
    m_pMidFilterKill = new ControlObject(ConfigKey(group, "filterMidKill"),this);
    m_pHighFilterKill = new ControlObject(ConfigKey(group, "filterHighKill"),this);
    m_pSpeed = new ControlObject(ConfigKey(group, "rate"),this);
    m_pPitchAdjust = new ControlObject(ConfigKey(group, "pitch_adjust"),this);
}
