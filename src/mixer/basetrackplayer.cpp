#include <QMessageBox>

#include "mixer/basetrackplayer.h"
#include "mixer/playerinfo.h"

#include "control/controlobject.h"
#include "control/controlpotmeter.h"
#include "track/track.h"
#include "sources/soundsourceproxy.h"
#include "engine/enginebuffer.h"
#include "engine/enginedeck.h"
#include "engine/enginemaster.h"
#include "track/beatgrid.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "analyzer/analyzerqueue.h"
#include "util/platform.h"
#include "util/sandbox.h"
#include "effects/effectsmanager.h"
#include "vinylcontrol/defs_vinylcontrol.h"
#include "engine/sync/enginesync.h"

TrackPlayer::TrackPlayer(
    QObject* pParent,
    UserSettingsPointer pConfig,
    EngineMaster* pMixingEngine,
    EffectsManager* pEffectsManager,
    EngineChannel::ChannelOrientation defaultOrientation,
    const QString& group,
    bool defaultMaster,
    bool defaultHeadphones)
        : BasePlayer(pParent, group),
          m_pConfig(pConfig),
          m_pEngineMaster(pMixingEngine),
          m_pLoadedTrack(),
          m_pLowFilter(nullptr),
          m_pMidFilter(nullptr),
          m_pHighFilter(nullptr),
          m_pLowFilterKill(nullptr),
          m_pMidFilterKill(nullptr),
          m_pHighFilterKill(nullptr),
          m_pRateSlider(nullptr),
          m_pPitchAdjust(nullptr),
          m_replaygainPending(false)
{
    auto channelGroup = pMixingEngine->registerChannelGroup(group);
    m_pChannel = new EngineDeck(pMixingEngine, channelGroup, pConfig, pMixingEngine,
                                pEffectsManager, defaultOrientation);

    m_pInputConfigured = new ControlProxy(group, "input_configured", this);
    m_pPassthroughEnabled = new ControlProxy(group, "passthrough", this);
    m_pPassthroughEnabled->connectValueChanged(SLOT(slotPassthroughEnabled(double)));
#ifdef __VINYLCONTROL__
    m_pVinylControlEnabled = new ControlProxy(group, "vinylcontrol_enabled", this);
    m_pVinylControlEnabled->connectValueChanged(SLOT(slotVinylControlEnabled(double)));
    m_pVinylControlStatus = new ControlProxy(group, "vinylcontrol_status", this);
#endif

    auto pEngineBuffer = m_pChannel->getEngineBuffer();
    pMixingEngine->addChannel(m_pChannel);

    // Set the routing option defaults for the master and headphone mixes.
    m_pChannel->setMaster(defaultMaster);
    m_pChannel->setPfl(defaultHeadphones);

    // Connect our signals and slots with the EngineBuffer's signals and
    // slots. This will let us know when the reader is done loading a track, and
    // let us request that the reader load a track.
    connect(pEngineBuffer, SIGNAL(trackLoaded(TrackPointer, TrackPointer)),
            this, SLOT(slotTrackLoaded(TrackPointer, TrackPointer)));
    connect(pEngineBuffer, SIGNAL(trackLoadFailed(TrackPointer, QString)),
            this, SLOT(slotLoadFailed(TrackPointer, QString)));

    // Get loop point control objects
    m_pLoopInPoint = new ControlProxy( getGroup(),"loop_start_position", this);
    m_pLoopOutPoint = new ControlProxy( getGroup(),"loop_end_position", this);

    // Duration of the current song, we create this one because nothing else does.
    m_pDuration = new ControlObject(ConfigKey(getGroup(), "duration"),this);

    // Waveform controls
    m_pWaveformZoom = new ControlPotmeter(ConfigKey(group, "waveform_zoom"),this,
                                          WaveformWidgetRenderer::s_waveformMinZoom,
                                          WaveformWidgetRenderer::s_waveformMaxZoom);
    m_pWaveformZoom->set(1.0);
    m_pWaveformZoom->setStepCount(WaveformWidgetRenderer::s_waveformMaxZoom -
            WaveformWidgetRenderer::s_waveformMinZoom);
    m_pWaveformZoom->setSmallStepCount(WaveformWidgetRenderer::s_waveformMaxZoom -
            WaveformWidgetRenderer::s_waveformMinZoom);

    m_pEndOfTrack = new ControlObject(ConfigKey(group, "end_of_track"),this);
    m_pEndOfTrack->set(0.);

    m_pPreGain = new ControlProxy(group, "pregain", this);
    //BPM of the current song
    m_pBPM = new ControlProxy(group, "file_bpm", this);
    m_pKey = new ControlProxy(group, "file_key", this);

    m_pReplayGain = new ControlProxy(group, "replaygain", this);
    m_pPlay = new ControlProxy(group, "play", this);
    m_pPlay->connectValueChanged(SLOT(slotPlayToggled(double)));
}
TrackPlayer::~TrackPlayer()
{
    if (m_pLoadedTrack) {
        emit(loadingTrack(TrackPointer(), m_pLoadedTrack));
        disconnect(m_pLoadedTrack.data(), 0, m_pBPM, 0);
        disconnect(m_pLoadedTrack.data(), 0, this, 0);
        disconnect(m_pLoadedTrack.data(), 0, m_pKey, 0);
        m_pLoadedTrack.clear();
    }

    delete m_pDuration;
    delete m_pWaveformZoom;
    delete m_pEndOfTrack;
}

void TrackPlayer::slotLoadTrack(TrackPointer pNewTrack, bool bPlay)
{
    qDebug() << "TrackPlayer::slotLoadTrack";
    // Before loading the track, ensure we have access. This uses lazy
    // evaluation to make sure track isn't nullptr before we dereference it.
    if (!pNewTrack.isNull() && !Sandbox::askForAccess(pNewTrack->getCanonicalLocation())) {
        // We don't have access.
        return;
    }
    auto pOldTrack = m_pLoadedTrack;
    // Disconnect the old track's signals.
    if (m_pLoadedTrack) {
        // Save the loops that are currently set in a loop cue. If no loop cue is
        // currently on the track, then create a new one.
        auto loopStart = int(m_pLoopInPoint->get());
        auto loopEnd = int(m_pLoopOutPoint->get());
        if (loopStart != -1 && loopEnd != -1 &&
            even(loopStart) && even(loopEnd) && loopStart <= loopEnd) {
            CuePointer pLoopCue;
            for(auto pCue : m_pLoadedTrack->getCuePoints()){
                if (pCue->getType() == Cue::LOOP)
                    pLoopCue = pCue;
            }
            if (!pLoopCue) {
                pLoopCue = m_pLoadedTrack->addCue();
                pLoopCue->setType(Cue::LOOP);
            }
            pLoopCue->setPosition(loopStart);
            pLoopCue->setLength(loopEnd - loopStart);
        }
        // WARNING: Never. Ever. call bare disconnect() on an object. Mixxx
        // relies on signals and slots to get tons of things done. Don't
        // randomly disconnect things.
        disconnect(m_pLoadedTrack.data(), 0, m_pBPM, 0);
        disconnect(m_pLoadedTrack.data(), 0, this, 0);
        disconnect(m_pLoadedTrack.data(), 0, m_pKey, 0);
        // Do not reset m_pReplayGain here, because the track might be still
        // playing and the last buffer will be processed.
        m_pPlay->set(0.0);
    }
    m_pLoadedTrack = pNewTrack;
    if (m_pLoadedTrack) {
        // Listen for updates to the file's BPM
        connect(m_pLoadedTrack.data(), SIGNAL(bpmUpdated(double)),m_pBPM, SLOT(set(double)));
        connect(m_pLoadedTrack.data(), SIGNAL(keyUpdated(double)),m_pKey, SLOT(set(double)));
        // Listen for updates to the file's Replay Gain
        connect(m_pLoadedTrack.data(), SIGNAL(ReplayGainUpdated(mixxx::ReplayGain)),
                this, SLOT(slotSetReplayGain(mixxx::ReplayGain)));
    }
    // Request a new track from EngineBuffer and wait for slotTrackLoaded()
    // call.
    auto pEngineBuffer = m_pChannel->getEngineBuffer();
    pEngineBuffer->loadTrack(pNewTrack, bPlay);
    // Causes the track's data to be saved back to the library database and
    // for all the widgets to change the track and update themselves.
    emit(loadingTrack(pNewTrack, pOldTrack));
}

void TrackPlayer::slotLoadFailed(TrackPointer track, QString reason)
{
    // Note: This slot can be a load failure from the current track or a
    // a delayed signal from a previous load.
    // We have probably received a slotTrackLoaded signal, of an old track that
    // was loaded before. Here we must unload the
    // We must unload the track m_pLoadedTrack as well
    if (track == m_pLoadedTrack) {
        qDebug() << "Failed to load track" << track->getLocation() << reason;
        slotTrackLoaded(TrackPointer(), track);
    } else if (!track.isNull()) {
        qDebug() << "Stray failed to load track" << track->getLocation() << reason;
    } else {
        qDebug() << "Failed to load track (nullptr track object)" << reason;
    }
    // Alert user.
    QMessageBox::warning(nullptr, tr("Couldn't load track."), reason);
}

void TrackPlayer::slotTrackLoaded(TrackPointer pNewTrack,
                                          TrackPointer pOldTrack) {
    qDebug() << "TrackPlayer::slotTrackLoaded";
    if (pNewTrack.isNull() && !pOldTrack.isNull() && pOldTrack == m_pLoadedTrack) {
        // eject Track
        // WARNING: Never. Ever. call bare disconnect() on an object. Mixxx
        // relies on signals and slots to get tons of things done. Don't
        // randomly disconnect things.
        // m_pLoadedTrack->disconnect();
        disconnect(m_pLoadedTrack.data(), 0, m_pBPM, 0);
        disconnect(m_pLoadedTrack.data(), 0, this, 0);
        disconnect(m_pLoadedTrack.data(), 0, m_pKey, 0);

        // Causes the track's data to be saved back to the library database and
        // for all the widgets to change the track and update themselves.
        emit(loadingTrack(pNewTrack, pOldTrack));
        m_pDuration->set(0);
        m_pBPM->set(0);
        m_pKey->set(0);
        setReplayGain(0);
        m_pLoopInPoint->set(-1);
        m_pLoopOutPoint->set(-1);
        m_pLoadedTrack.clear();
        emit(playerEmpty());
    } else if (!pNewTrack.isNull() && pNewTrack == m_pLoadedTrack) {
        // Successful loaded a new track
        // Reload metadata from file, but only if required
        SoundSourceProxy(m_pLoadedTrack).loadTrackMetadata();

        // Update the BPM and duration values that are stored in ControlObjects
        m_pDuration->set(m_pLoadedTrack->getDuration());
        m_pBPM->set(m_pLoadedTrack->getBpm());
        m_pKey->set(m_pLoadedTrack->getKey());
        setReplayGain(m_pLoadedTrack->getReplayGain().getRatio());

        // Clear loop
        // It seems that the trick is to first clear the loop out point, and then
        // the loop in point. If we first clear the loop in point, the loop out point
        // does not get cleared.
        m_pLoopOutPoint->set(-1);
        m_pLoopInPoint->set(-1);

        for(auto && pCue : pNewTrack->getCuePoints()) {
            if (pCue->getType() == Cue::LOOP) {
                auto loopStart = pCue->getPosition();
                auto loopEnd = loopStart + pCue->getLength();
                if (loopStart != -1 && loopEnd != -1 && even(loopStart) && even(loopEnd)) {
                    m_pLoopInPoint->set(loopStart);
                    m_pLoopOutPoint->set(loopEnd);
                    break;
                }
            }
        }
        if(m_pConfig->getValueString(ConfigKey("[Mixer Profile]", "EqAutoReset"), 0).toInt()) {
            if (m_pLowFilter) {
                m_pLowFilter->set(1.0);
            }
            if (m_pMidFilter) {
                m_pMidFilter->set(1.0);
            }
            if (m_pHighFilter) {
                m_pHighFilter->set(1.0);
            }
            if (m_pLowFilterKill) {
                m_pLowFilterKill->set(0.0);
            }
            if (m_pMidFilterKill) {
                m_pMidFilterKill->set(0.0);
            }
            if (m_pHighFilterKill) {
                m_pHighFilterKill->set(0.0);
            }
            m_pPreGain->set(1.0);
        }
        auto reset = m_pConfig->getValueString(ConfigKey("[Controls]", "SpeedAutoReset"),QString("%1").arg(RESET_PITCH)).toInt();
        if (reset == RESET_SPEED || reset == RESET_PITCH_AND_SPEED) {
            // Avoid reseting speed if master sync is enabled and other decks with sync enabled
            // are playing, as this would change the speed of already playing decks.
            if (! m_pEngineMaster->getEngineSync()->otherSyncedPlaying(getGroup())) {
                if (m_pRateSlider) {
                    m_pRateSlider->set(0.0);
                }
            }
        }
        if (reset == RESET_PITCH || reset == RESET_PITCH_AND_SPEED) {
            if (m_pPitchAdjust) {
                m_pPitchAdjust->set(0.0);
            }
        }
        emit(newTrackLoaded(m_pLoadedTrack));
    } else {
        // this is the result from an outdated load or unload signal
        // A new load is already pending
        // Ignore this signal and wait for the new one
        qDebug() << "stray TrackPlayer::slotTrackLoaded()";
    }

    // Update the PlayerInfo class that is used in EngineBroadcast to replace
    // the metadata of a stream
    PlayerInfo::instance().setTrackInfo(getGroup(), m_pLoadedTrack);
}
TrackPointer TrackPlayer::getLoadedTrack() const
{
    return m_pLoadedTrack;
}
void TrackPlayer::slotSetReplayGain(mixxx::ReplayGain replayGain)
{
    // Do not change replay gain when track is playing because
    // this may lead to an unexpected volume change
    if (m_pPlay->get() == 0.0) {
        setReplayGain(replayGain.getRatio());
    } else {
        m_replaygainPending = true;
    }
}

void TrackPlayer::slotPlayToggled(double v)
{
    if (!v && m_replaygainPending) {
        setReplayGain(m_pLoadedTrack->getReplayGain().getRatio());
    }
}

EngineDeck* TrackPlayer::getEngineDeck() const
{
    return m_pChannel;
}

void TrackPlayer::setupEqControls()
{
    auto group = getGroup();
    m_pLowFilter = new ControlProxy(group, "filterLow", this);
    m_pMidFilter = new ControlProxy(group, "filterMid", this);
    m_pHighFilter = new ControlProxy(group, "filterHigh", this);
    m_pLowFilterKill = new ControlProxy(group, "filterLowKill", this);
    m_pMidFilterKill = new ControlProxy(group, "filterMidKill", this);
    m_pHighFilterKill = new ControlProxy(group, "filterHighKill", this);
    m_pRateSlider = new ControlProxy(group, "rate", this);
    m_pPitchAdjust = new ControlProxy(group, "pitch_adjust", this);
}

void TrackPlayer::slotPassthroughEnabled(double v)
{
    auto configured = m_pInputConfigured->toBool();
    auto passthrough = v > 0.0;

    // Warn the user if they try to enable passthrough on a player with no
    // configured input.
    if (!configured && passthrough) {
        m_pPassthroughEnabled->set(0.0);
        emit(noPassthroughInputConfigured());
    }
}

void TrackPlayer::slotVinylControlEnabled(double v) {
#ifdef __VINYLCONTROL__
    auto configured = m_pInputConfigured->toBool();
    auto vinylcontrol_enabled = v > 0.0;

    // Warn the user if they try to enable vinyl control on a player with no
    // configured input.
    if (!configured && vinylcontrol_enabled) {
        m_pVinylControlEnabled->set(0.0);
        m_pVinylControlStatus->set(VINYL_STATUS_DISABLED);
        emit(noVinylControlInputConfigured());
    }
#endif
}
void TrackPlayer::setReplayGain(double value)
{
    m_pReplayGain->set(value);
    m_replaygainPending = false;
}
