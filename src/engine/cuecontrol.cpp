// cuecontrol.cpp
// Created 11/5/2009 by RJ Ryan (rryan@mit.edu)

#include <QMutexLocker>
#include <QStringBuilder>

#include "engine/cuecontrol.h"

#include "control/controlobject.h"
#include "control/controlpushbutton.h"
#include "control/controlindicator.h"
#include "vinylcontrol/defs_vinylcontrol.h"

// TODO: Convert these doubles to a standard enum
// and convert elseif logic to switch statements
static constexpr double CUE_MODE_MIXXX = 0.0;
static constexpr double CUE_MODE_PIONEER = 1.0;
static constexpr double CUE_MODE_DENON = 2.0;
static constexpr double CUE_MODE_NUMARK = 3.0;
static constexpr double CUE_MODE_MIXXX_NO_BLINK = 4.0;
static constexpr double CUE_MODE_CUP = 5.0;

CueControl::CueControl(QString group,
                       UserSettingsPointer pConfig) :
        EngineControl(group, pConfig),
        m_bPreviewing(false),
        m_playing(new ControlObject(ConfigKey(group, "playing"),this)),
        m_pPlayButton(new ControlObject(ConfigKey(group, "play"),this)),
        m_pStopButton(new ControlObject(ConfigKey(group, "stop"),this)),
        m_iCurrentlyPreviewingHotcues(0),
        m_bypassCueSetByPlay(false),
        m_iNumHotCues(NUM_HOT_CUES),
        m_pLoadedTrack(),
        m_mutex(QMutex::Recursive) {
    // To silence a compiler warning about CUE_MODE_PIONEER.
    Q_UNUSED(CUE_MODE_PIONEER);
    createControls();

    m_pTrackSamples = new ControlObject(ConfigKey(group, "track_samples"),this);

    m_pQuantizeEnabled = new ControlObject(ConfigKey(group, "quantize"),this);

    m_pNextBeat = new ControlObject(ConfigKey(group, "beat_next"),this);
    m_pClosestBeat = new ControlObject(ConfigKey(group, "beat_closest"),this);

    m_pCuePoint = new ControlObject(ConfigKey(group, "cue_point"),this);
    m_pCuePoint->set(-1.0);

    m_pCueMode = new ControlObject(ConfigKey(group, "cue_mode"),this);

    m_pCueSet = new ControlPushButton(ConfigKey(group, "cue_set"),this);
    m_pCueSet->setButtonMode(ControlPushButton::TRIGGER);
    connect(m_pCueSet, SIGNAL(valueChanged(double)),this, SLOT(cueSet(double)),Qt::AutoConnection);

    m_pCueGoto = new ControlPushButton(ConfigKey(group, "cue_goto"),this);
    connect(m_pCueGoto, SIGNAL(valueChanged(double)),this, SLOT(cueGoto(double)),Qt::AutoConnection);

    m_pCueGotoAndPlay =
            new ControlPushButton(ConfigKey(group, "cue_gotoandplay"),this);
    connect(m_pCueGotoAndPlay, SIGNAL(valueChanged(double)),this, SLOT(cueGotoAndPlay(double)),Qt::AutoConnection);

    m_pCuePlay = new ControlPushButton(ConfigKey(group, "cue_play"),this);
    connect(m_pCuePlay, SIGNAL(valueChanged(double)),this, SLOT(cuePlay(double)),Qt::AutoConnection);

    m_pCueGotoAndStop = new ControlPushButton(ConfigKey(group, "cue_gotoandstop"),this);
    connect(m_pCueGotoAndStop, SIGNAL(valueChanged(double)),this, SLOT(cueGotoAndStop(double)),Qt::AutoConnection);

    m_pCuePreview = new ControlPushButton(ConfigKey(group, "cue_preview"),this);
    connect(m_pCuePreview, SIGNAL(valueChanged(double)),this, SLOT(cuePreview(double)),Qt::AutoConnection);

    m_pCueCDJ = new ControlPushButton(ConfigKey(group, "cue_cdj"),this);
    connect(m_pCueCDJ, SIGNAL(valueChanged(double)),this, SLOT(cueCDJ(double)),Qt::AutoConnection);

    m_pCueDefault = new ControlPushButton(ConfigKey(group, "cue_default"),this);
    connect(m_pCueDefault, SIGNAL(valueChanged(double)),this, SLOT(cueDefault(double)),Qt::AutoConnection);

    m_pPlayStutter = new ControlPushButton(ConfigKey(group, "play_stutter"),this);
    connect(m_pPlayStutter, SIGNAL(valueChanged(double)),this, SLOT(playStutter(double)),Qt::AutoConnection);

    m_pCueIndicator = new ControlIndicator(ConfigKey(group, "cue_indicator"));
    m_pPlayIndicator = new ControlIndicator(ConfigKey(group, "play_indicator"));

    m_pVinylControlEnabled = new ControlProxy(group, "vinylcontrol_enabled",this);
    m_pVinylControlMode = new ControlProxy(group, "vinylcontrol_mode",this);
}

CueControl::~CueControl()
{
    qDeleteAll(m_hotcueControl);
}

void CueControl::createControls() {
    for (auto i = 0; i < m_iNumHotCues; ++i) {
        auto pControl = new HotcueControl(this,getGroup(), i);

        connect(pControl, SIGNAL(hotcuePositionChanged(HotcueControl*, double)),
                this, SLOT(hotcuePositionChanged(HotcueControl*, double)),
                Qt::AutoConnection);
        connect(pControl, SIGNAL(hotcueSet(HotcueControl*, double)),
                this, SLOT(hotcueSet(HotcueControl*, double)),
                Qt::AutoConnection);
        connect(pControl, SIGNAL(hotcueGoto(HotcueControl*, double)),
                this, SLOT(hotcueGoto(HotcueControl*, double)),
                Qt::AutoConnection);
        connect(pControl, SIGNAL(hotcueGotoAndPlay(HotcueControl*, double)),
                this, SLOT(hotcueGotoAndPlay(HotcueControl*, double)),
                Qt::AutoConnection);
        connect(pControl, SIGNAL(hotcueGotoAndStop(HotcueControl*, double)),
                this, SLOT(hotcueGotoAndStop(HotcueControl*, double)),
                Qt::AutoConnection);
        connect(pControl, SIGNAL(hotcueActivate(HotcueControl*, double)),
                this, SLOT(hotcueActivate(HotcueControl*, double)),
                Qt::AutoConnection);
        connect(pControl, SIGNAL(hotcueActivatePreview(HotcueControl*, double)),
                this, SLOT(hotcueActivatePreview(HotcueControl*, double)),
                Qt::AutoConnection);
        connect(pControl, SIGNAL(hotcueClear(HotcueControl*, double)),
                this, SLOT(hotcueClear(HotcueControl*, double)),
                Qt::AutoConnection);

        m_hotcueControl.append(pControl);
    }
}

void CueControl::attachCue(CuePointer pCue, int hotCue)
{
    if(auto pControl = m_hotcueControl.value(hotCue, nullptr)){
        if (pControl->getCue())
            detachCue(pControl->getHotcueNumber());
        connect(pCue.data(), SIGNAL(updated()),this, SLOT(cueUpdated()),Qt::AutoConnection);
        pControl->getPosition()->set(pCue->getPosition());
        pControl->getEnabled()->set(pCue->getPosition() == -1 ? 0.0 : 1.0);
        // set pCue only if all other data is in place
        // because we have a null check for valid data else where in the code
        pControl->setCue(pCue);
    }

}

void CueControl::detachCue(int hotCue)
{
    if(auto  pControl = m_hotcueControl.value(hotCue, nullptr)){
        if(auto pCue = pControl->getCue()){
            disconnect(pCue.data(), 0, this, 0);
            // clear pCue first because we have a null check for valid data else where
            // in the code
            pControl->setCue(CuePointer());
            pControl->getPosition()->set(-1); // invalidate position for hintReader()
            pControl->getEnabled()->set(0);
        }
    }
}

void CueControl::trackLoaded(TrackPointer pNewTrack, TrackPointer pOldTrack)
{
    Q_UNUSED(pOldTrack);
    QMutexLocker lock(&m_mutex);

    if (m_pLoadedTrack) {
        disconnect(m_pLoadedTrack.data(), 0, this, 0);
        for (int i = 0; i < m_iNumHotCues; ++i) {
            detachCue(i);
        }

        // Store the cue point in a load cue.
        auto cuePoint = m_pCuePoint->get();
        if (cuePoint != -1 && cuePoint != 0.0) {
            CuePointer loadCue;
            for(auto pCue : m_pLoadedTrack->getCuePoints()) {
                if (pCue->getType() == Cue::LOAD) {
                    loadCue = pCue;
                    break;
                }
            }
            if (!loadCue) {
                loadCue = m_pLoadedTrack->addCue();
                loadCue->setType(Cue::LOAD);
                loadCue->setLength(0);
            }
            loadCue->setPosition(cuePoint);
        }
        m_pCueIndicator->setBlinkValue(ControlIndicator::OFF);
        m_pCuePoint->set(-1.0);
        m_pLoadedTrack.clear();
    }
    if (pNewTrack.isNull())
        return;

    m_pLoadedTrack = pNewTrack;
    connect(pNewTrack.data(), SIGNAL(cuesUpdated()),this, SLOT(trackCuesUpdated()),Qt::AutoConnection);

    CuePointer loadCue;
    for(auto pCue :  pNewTrack->getCuePoints()) {
        if (pCue->getType() == Cue::LOAD) {
            loadCue = pCue;
        } else if (pCue->getType() != Cue::CUE) {
            continue;
        }
        auto hotcue = pCue->getHotCue();
        if (hotcue != -1)
            attachCue(pCue, hotcue);
    }
    auto loadCuePoint = 0.0;
    // If cue recall is ON in the prefs, then we're supposed to seek to the cue
    // point on song load. Note that [Controls],cueRecall == 0 corresponds to "ON", not OFF.
    auto cueRecall = (getConfig()->getValueString(ConfigKey("[Controls]","CueRecall"), "0").toInt() == 0);
    if (loadCue ) {
        m_pCuePoint->set(loadCue->getPosition());
        if (cueRecall) {
            loadCuePoint = loadCue->getPosition();
        }
    } else {
        // If no cue point is stored, set one at track start
        m_pCuePoint->set(0.0);
    }
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    // If cueRecall is on, seek to it even if we didn't find a cue value (we'll
    // seek to 0.
    if (cueRecall) {
        seekExact(loadCuePoint);
    } else if (!(m_pVinylControlEnabled->get() &&
            m_pVinylControlMode->get() == MIXXX_VCMODE_ABSOLUTE)) {
        // If cuerecall is off, seek to zero unless
        // vinylcontrol is on and set to absolute.  This allows users to
        // load tracks and have the needle-drop be maintained.
        seekExact(0.0);
    }
}

void CueControl::cueUpdated() {
    //QMutexLocker lock(&m_mutex);
    // We should get a trackCuesUpdated call anyway, so do nothing.
}

void CueControl::trackCuesUpdated()
{
    QMutexLocker lock(&m_mutex);
    QSet<int> active_hotcues;

    if (!m_pLoadedTrack)
        return;

    for(auto pCue :m_pLoadedTrack->getCuePoints()) {
        if (pCue->getType() != Cue::CUE && pCue->getType() != Cue::LOAD)
            continue;
        auto hotcue = pCue->getHotCue();
        if (hotcue != -1) {
            if(auto pControl = m_hotcueControl.value(hotcue, NULL)) {
                auto pOldCue = pControl->getCue();
                // If the old hotcue is different than this one.
                if (pOldCue != pCue) {
                    // If the old hotcue exists, detach it
                    if (pOldCue)
                        detachCue(hotcue);
                    attachCue(pCue, hotcue);
                } else {
                    // If the old hotcue is the same, then we only need to update
                    auto dOldPosition = pControl->getPosition()->get();
                    auto dOldEnabled = pControl->getEnabled()->get();
                    auto dPosition = pCue->getPosition();
                    auto dEnabled = dPosition == -1 ? 0.0 : 1.0;
                    if (dEnabled != dOldEnabled) {
                        pControl->getEnabled()->set(dEnabled);
                    }
                    if (dPosition != dOldPosition) {
                        pControl->getPosition()->set(dPosition);
                    }
                }
                // Add the hotcue to the list of active hotcues
                active_hotcues.insert(hotcue);
            }
        }
    }
    // Detach all hotcues that are no longer present
    for (auto i = 0; i < m_iNumHotCues; ++i) {
        if (!active_hotcues.contains(i))
            detachCue(i);
    }
}
void CueControl::hotcueSet(HotcueControl* pControl, double v)
{
    //qDebug() << "CueControl::hotcueSet" << v;

    if (!v)
        return;

    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack)
        return;

    auto hotcue = pControl->getHotcueNumber();
    hotcueClear(pControl, v);
    auto pCue = m_pLoadedTrack->addCue();
    auto cuePosition = (m_pQuantizeEnabled->get() > 0.0 && m_pClosestBeat->get() != -1) ?
            floor(m_pClosestBeat->get()) : floor(getCurrentSample());
    if (!even(static_cast<int>(cuePosition)))
        cuePosition--;
    pCue->setPosition(cuePosition);
    pCue->setHotCue(hotcue);
    pCue->setLabel("");
    pCue->setType(Cue::CUE);
    // TODO(XXX) deal with spurious signals
    attachCue(pCue, hotcue);
    // If quantize is enabled and we are not playing, jump to the cue point
    // since it's not necessarily where we currently are. TODO(XXX) is this
    // potentially invalid for vinyl control?
    auto playing = m_playing->toBool();//m_pPlayButton->toBool();
    if (!playing && m_pQuantizeEnabled->get() > 0.0) {
        lock.unlock();  // prevent deadlock.
        // Enginebuffer will quantize more exactly than we can.
        seekAbs(cuePosition);
    }
}

void CueControl::hotcueGoto(HotcueControl* pControl, double v)
{
    if (!v)
        return;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack)
        return;

    auto pCue = pControl->getCue();
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    if (pCue) {
        auto position = pCue->getPosition();
        if (position != -1)
            seekAbs(position);
    }
}

void CueControl::hotcueGotoAndStop(HotcueControl* pControl, double v)
{
    if (!v)
        return;
    auto pCue = CuePointer{};
    {
        QMutexLocker lock(&m_mutex);
        if (!m_pLoadedTrack)
            return;
        auto pCue = pControl->getCue();
        // Need to unlock before emitting any signals to prevent deadlock.
    }
    if (pCue) {
        auto position = pCue->getPosition();
        if (position != -1) {
            m_pPlayButton->set(0.0);
            seekExact(position);
        }
    }
}
void CueControl::hotcueGotoAndPlay(HotcueControl* pControl, double v)
{
    if (!v)
        return;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack)
        return;
    auto pCue = pControl->getCue();

    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();

    if (pCue) {
        auto position = pCue->getPosition();
        if (position != -1) {
            seekAbs(position);
            // don't move the cue point to the hot cue point in DENON mode
            m_bypassCueSetByPlay = true;
            m_pPlayButton->set(1.0);
        }
    }
}

void CueControl::hotcueActivate(HotcueControl* pControl, double v)
{
    //qDebug() << "CueControl::hotcueActivate" << v;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack)
        return;
    auto pCue = pControl->getCue();

    lock.unlock();

    if (pCue) {
        if (v) {
            if (pCue->getPosition() == -1) {
                hotcueSet(pControl, v);
            } else {
                if (!m_iCurrentlyPreviewingHotcues && !m_bPreviewing && m_pPlayButton->toBool()) {
                    hotcueGoto(pControl, v);
                } else {
                    hotcueActivatePreview(pControl, v);
                }
            }
        } else {
            if (pCue->getPosition() != -1) {
                hotcueActivatePreview(pControl, v);
            }
        }
    } else {
        if (v) {
            // just in case
            hotcueSet(pControl, v);
        } else if (m_iCurrentlyPreviewingHotcues) {
            // The cue is non-existent, yet we got a release for it and are
            // currently previewing a hotcue. This is indicative of a corner
            // case where the cue was detached while we were pressing it. Let
            // hotcueActivatePreview handle it.
            hotcueActivatePreview(pControl, v);
        }
    }
}
void CueControl::hotcueActivatePreview(HotcueControl* pControl, double v)
{
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack) {
        return;
    }
    auto pCue = pControl->getCue();

    if (v) {
        if (pCue && pCue->getPosition() != -1) {
            m_iCurrentlyPreviewingHotcues++;
            auto iPosition = pCue->getPosition();
            m_bypassCueSetByPlay = true;
            m_pPlayButton->set(1.0);
            pControl->setPreviewing(true);
            pControl->setPreviewingPosition(iPosition);
            // Need to unlock before emitting any signals to prevent deadlock.
            lock.unlock();
            seekAbs(iPosition);
        }
    } else if (m_iCurrentlyPreviewingHotcues) {
        // This is a activate release and we are previewing at least one
        // hotcue. If this hotcue is previewing:
        if (pControl->isPreviewing()) {
            // Mark this hotcue as not previewing.
            auto iPosition = pControl->getPreviewingPosition();
            pControl->setPreviewing(false);
            pControl->setPreviewingPosition(-1);

            // If this is the last hotcue to leave preview.
            if (--m_iCurrentlyPreviewingHotcues == 0 && !m_bPreviewing) {
                m_pPlayButton->set(0.0);
                // Need to unlock before emitting any signals to prevent deadlock.
                lock.unlock();
                seekExact(iPosition);
            }
        }
    }
}

void CueControl::hotcueClear(HotcueControl* pControl, double v)
{
    if (!v)
        return;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack)
        return;
    if(auto pCue = pControl->getCue()){
        pCue->setHotCue(-1);
    }
    detachCue(pControl->getHotcueNumber());
}

void CueControl::hotcuePositionChanged(HotcueControl* pControl, double newPosition)
{
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack)
        return;

    if(auto pCue = pControl->getCue()){
        // Setting the position to -1 is the same as calling hotcue_x_clear
        if (newPosition == -1) {
            pCue->setHotCue(-1);
            detachCue(pControl->getHotcueNumber());
        } else if (newPosition > 0 && newPosition < m_pTrackSamples->get()) {
            auto position = int(newPosition);
            // People writing from MIDI land, elsewhere might be careless.
            if (position % 2 != 0) {
                position--;
            }
            pCue->setPosition(position);
        }
    }
}

void CueControl::hintReader(HintVector* pHintList)
{
    Hint cue_hint;
    auto cuePoint = m_pCuePoint->get();
    if (cuePoint >= 0) {
        cue_hint.sample = m_pCuePoint->get();
        cue_hint.length = 0;
        cue_hint.priority = 10;
        pHintList->append(cue_hint);
    }

    // this is called from the engine thread
    // it is no locking required, because m_hotcueControl is filled during the
    // constructor and getPosition()->get() is a ControlObject
    for (auto it = m_hotcueControl.constBegin();it != m_hotcueControl.constEnd(); ++it) {
        auto pControl = *it;
        auto position = pControl->getPosition()->get();
        if (position != -1) {
            cue_hint.sample = position;
            if (cue_hint.sample % 2 != 0)
                cue_hint.sample--;
            cue_hint.length = 0;
            cue_hint.priority = 10;
            pHintList->append(cue_hint);
        }
    }
}
void CueControl::saveCuePoint(double cuePoint)
{
    if (m_pLoadedTrack)
        m_pLoadedTrack->setCuePoint(cuePoint);
}

// Moves the cue point to current position or to closest beat in case
// quantize is enabled
void CueControl::cueSet(double v)
{
    if (!v)
        return;

    QMutexLocker lock(&m_mutex);
    auto cue = (m_pQuantizeEnabled->get() > 0.0 && m_pClosestBeat->get() != -1) ?
            floor(m_pClosestBeat->get()) : floor(getCurrentSample());
    if (!even(static_cast<int>(cue)))
        cue--;
    m_pCuePoint->set(cue);
    saveCuePoint(cue);
}

void CueControl::cueGoto(double v)
{
    if (!v)
        return;
    QMutexLocker lock(&m_mutex);
    // Seek to cue point
    auto cuePoint = m_pCuePoint->get();
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    seekAbs(cuePoint);
}

void CueControl::cueGotoAndPlay(double v)
{
    if (!v)
        return;
    cueGoto(v);
    QMutexLocker lock(&m_mutex);
    // Start playing if not already
    if (!m_playing->toBool()){//m_pPlayButton->toBool()) {
        // cueGoto is processed asynchrony.
        // avoid a wrong cue set if seek by cueGoto is still pending
        m_bypassCueSetByPlay = true;
        m_pStopButton->set(0.0);
        m_pPlayButton->set(1.0);
    }
}

void CueControl::cueGotoAndStop(double v)
{
    if (!v)
        return;
    QMutexLocker lock(&m_mutex);
    m_pPlayButton->set(0.0);
    m_pStopButton->set(1.0);
    auto cuePoint = m_pCuePoint->get();
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    seekExact(cuePoint);
}

void CueControl::cuePreview(double v)
{
    QMutexLocker lock(&m_mutex);
    if (v) {
        m_bPreviewing = true;
        m_bypassCueSetByPlay = true;
        m_pPlayButton->set(1.0);
    } else if (!v && m_bPreviewing) {
        m_bPreviewing = false;
        if (!m_iCurrentlyPreviewingHotcues) {
            m_pPlayButton->set(0.0);
        } else {
            return;
        }
    } else {
        return;
    }
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    seekAbs(m_pCuePoint->get());
}

void CueControl::cueCDJ(double v)
{
    // This is how Pioneer cue buttons work:
    // If pressed while playing, stop playback and go to cue.
    // If pressed while stopped and at cue, play while pressed.
    // If pressed while stopped and not at cue, set new cue point.
    // If play is pressed while holding cue, the deck is now playing. (Handled in playFromCuePreview().)

    QMutexLocker lock(&m_mutex);
    auto playing = m_playing->toBool();//m_pPlayButton->toBool();

    if (v) {
        if (m_iCurrentlyPreviewingHotcues) {
            // we are already previewing by hotcues
            // just jump to cue point and continue previewing
            m_bPreviewing = true;
            lock.unlock();
            seekAbs(m_pCuePoint->get());
        } else if (playing || atEndPosition()) {
            // Jump to cue when playing or when at end position
            // Just in case.
            m_bPreviewing = false;
            m_pPlayButton->set(0.0);
            // Need to unlock before emitting any signals to prevent deadlock.
            lock.unlock();
            seekAbs(m_pCuePoint->get());
        } else if (isTrackAtCue()) {
            // pause at cue point
            m_bPreviewing = true;
            m_pPlayButton->set(1.0);
        } else {
            // Pause not at cue point and not at end position
            cueSet(v);
            // Just in case.
            m_bPreviewing = false;
            // If quantize is enabled, jump to the cue point since it's not
            // necessarily where we currently are
            if (m_pQuantizeEnabled->get() > 0.0) {
                lock.unlock();  // prevent deadlock.
                // Enginebuffer will quantize more exactly than we can.
                seekAbs(m_pCuePoint->get());
            }
        }
    } else if (m_bPreviewing) {
        m_bPreviewing = false;
        if (!m_iCurrentlyPreviewingHotcues) {
            m_pPlayButton->set(0.0);

            // Need to unlock before emitting any signals to prevent deadlock.
            lock.unlock();
            seekAbs(m_pCuePoint->get());
        }
    }
    // indicator may flash because the delayed adoption of seekAbs
    // Correct the Indicator set via play
    if (m_pLoadedTrack && !playing) {
        m_pCueIndicator->setBlinkValue(ControlIndicator::ON);
    } else {
        m_pCueIndicator->setBlinkValue(ControlIndicator::OFF);
    }
}

void CueControl::cueDenon(double v) {
    // This is how Denon DN-S 3700 cue buttons work:
    // If pressed go to cue and stop.
    // If pressed while stopped and at cue, play while pressed.
    // Cue Point is moved by play from pause
    QMutexLocker lock(&m_mutex);
    auto playing = m_playing->toBool();//(m_pPlayButton->toBool());
    if (v) {
        if (m_iCurrentlyPreviewingHotcues) {
            // we are already previewing by hotcues
            // just jump to cue point and continue previewing
            m_bPreviewing = true;
            lock.unlock();
            seekAbs(m_pCuePoint->get());
        } else if (!playing && isTrackAtCue()) {
            // pause at cue point
            m_bPreviewing = true;
            m_pPlayButton->set(1.0);
        } else {
            // Just in case.
            m_bPreviewing = false;
            m_pPlayButton->set(0.0);

            // Need to unlock before emitting any signals to prevent deadlock.
            lock.unlock();
            seekAbs(m_pCuePoint->get());
        }
    } else if (m_bPreviewing) {
        m_bPreviewing = false;
        if (!m_iCurrentlyPreviewingHotcues) {
            m_pPlayButton->set(0.0);
            // Need to unlock before emitting any signals to prevent deadlock.
            lock.unlock();
            seekAbs(m_pCuePoint->get());
        }
    }
}
void CueControl::cuePlay(double v)
{
    // This is how CUP button works:
    // If playing, press to go to cue and stop.
    // If stopped, press to set as cue point.
    // On release, start playing from cue point.


    QMutexLocker lock(&m_mutex);
    auto playing = m_playing->toBool();//(m_pPlayButton->toBool());

    // pressed
    if (v) {
        if (playing) {
            m_bPreviewing = false;
            m_pPlayButton->set(0.0);
            // Need to unlock before emitting any signals to prevent deadlock.
            lock.unlock();
            seekAbs(m_pCuePoint->get());
        } else if (!isTrackAtCue() && getCurrentSample() <= getTotalSamples()) {
            // Pause not at cue point and not at end position
            cueSet(v);
            // Just in case.
            m_bPreviewing = false;
        }
    } else if (isTrackAtCue()){
        m_bPreviewing = false;
        m_pPlayButton->set(1.0);
        lock.unlock();

    }
}

void CueControl::cueDefault(double v)
{
    auto cueMode = m_pCueMode->get();
    // Decide which cue implementation to call based on the user preference
    if (cueMode == CUE_MODE_DENON || cueMode == CUE_MODE_NUMARK) {
        cueDenon(v);
    } else if (cueMode == CUE_MODE_CUP) {
        cuePlay(v);
    } else {
        // The modes CUE_MODE_PIONEER and CUE_MODE_MIXXX are similar
        // are handled inside cueCDJ(v)
        // default to Pioneer mode
        cueCDJ(v);
    }
}

void CueControl::pause(double v) {
    QMutexLocker lock(&m_mutex);
    //qDebug() << "CueControl::pause()" << v;
    if (v != 0.0) {
        m_pPlayButton->set(0.0);
    }
}

void CueControl::playStutter(double v) {
    QMutexLocker lock(&m_mutex);
    //qDebug() << "playStutter" << v;
    if (v != 0.0) {
        if (m_playing->toBool()){
            cueGoto(1.0);
        } else {
            m_pPlayButton->set(1.0);
        }
    }
}

bool CueControl::updateIndicatorsAndModifyPlay(bool newPlay, bool playPossible)
{
    //qDebug() << "updateIndicatorsAndModifyPlay" << newPlay << playPossible
    //        << m_iCurrentlyPreviewingHotcues << m_bPreviewing;
    QMutexLocker lock(&m_mutex);
    auto cueMode = m_pCueMode->get();
    if ((cueMode == CUE_MODE_DENON || cueMode == CUE_MODE_NUMARK) &&
        newPlay && playPossible &&
//        !m_pPlayButton->toBool() &&
        !m_playing->toBool() &&
        !m_bypassCueSetByPlay) {
        // in Denon mode each play from pause moves the cue point
        // if not previewing
        cueSet(1.0);
    }
    m_bypassCueSetByPlay = false;

    // when previewing, "play" was set by cue button, a following toggle request
    // (play = 0.0) is used for latching play.
    auto previewing = false;
    if (m_bPreviewing || m_iCurrentlyPreviewingHotcues) {
        if (!newPlay) {
            // play latch request: stop previewing and go into normal play mode.
            m_bPreviewing = false;
            m_iCurrentlyPreviewingHotcues = 0;
            newPlay = true;
        } else {
            previewing = true;
        }
    }

    if (!playPossible) {
        // play not possible
        newPlay = false;
        m_pPlayIndicator->setBlinkValue(ControlIndicator::OFF);
        m_pStopButton->set(0.0);
    } else if (newPlay && !previewing) {
        // Play: Indicates a latched Play
        m_pPlayIndicator->setBlinkValue(ControlIndicator::ON);
        m_pStopButton->set(0.0);
    } else {
        // Pause:
        m_pStopButton->set(1.0);
        if (cueMode == CUE_MODE_DENON) {
            if (isTrackAtCue() || previewing) {
                m_pPlayIndicator->setBlinkValue(ControlIndicator::OFF);
            } else {
                // Flashing indicates that a following play would move cue point
                m_pPlayIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_500MS);
            }
        } else if (cueMode == CUE_MODE_MIXXX || cueMode == CUE_MODE_MIXXX_NO_BLINK ||
                   cueMode == CUE_MODE_NUMARK) {
            m_pPlayIndicator->setBlinkValue(ControlIndicator::OFF);
        } else {
            // Flashing indicates that play is possible in Pioneer mode
            m_pPlayIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_500MS);
        }
    }
    if (cueMode != CUE_MODE_DENON && cueMode != CUE_MODE_NUMARK) {
        if (m_pCuePoint->get() != -1) {
            if (newPlay == 0.0 && !isTrackAtCue() &&
                    !atEndPosition()) {
                if (cueMode == CUE_MODE_MIXXX) {
                    // in Mixxx mode Cue Button is flashing slow if CUE will move Cue point
                    m_pCueIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_500MS);
                } else if (cueMode == CUE_MODE_MIXXX_NO_BLINK) {
                    m_pCueIndicator->setBlinkValue(ControlIndicator::OFF);
                } else {
                    // in Pioneer mode Cue Button is flashing fast if CUE will move Cue point
                    m_pCueIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_250MS);
                }
            } else {
                m_pCueIndicator->setBlinkValue(ControlIndicator::OFF);
            }
        } else {
            m_pCueIndicator->setBlinkValue(ControlIndicator::OFF);
        }
    }
    m_pPlayStutter->set(newPlay ? 1.0 : 0.0);
    return newPlay;
}

void CueControl::updateIndicators()
{
    // No need for mutex lock because we are only touching COs.
    auto cueMode = m_pCueMode->get();

    if (cueMode == CUE_MODE_DENON || cueMode == CUE_MODE_NUMARK) {
        // Cue button is only lit at cue point
        auto playing = m_playing->toBool();//m_pPlayButton->toBool();
        if (isTrackAtCue()) {
            // at cue point
            if (!playing) {
                m_pCueIndicator->setBlinkValue(ControlIndicator::ON);
                m_pPlayIndicator->setBlinkValue(ControlIndicator::OFF);
            }
        } else {
            m_pCueIndicator->setBlinkValue(ControlIndicator::OFF);
            if (!playing) {
                if (!atEndPosition() && cueMode != CUE_MODE_NUMARK) {
                    // Play will move cue point
                    m_pPlayIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_500MS);
                } else {
                    // At track end
                    m_pPlayIndicator->setBlinkValue(ControlIndicator::OFF);
                }
            }
        }
    } else {
        // Here we have CUE_MODE_PIONEER or CUE_MODE_MIXXX
        // default to Pioneer mode
        if (!m_bPreviewing) {
            auto playing = m_playing->toBool();//m_pPlayButton->toBool();
            if (!playing) {
                if (!isTrackAtCue()) {
                    if (!atEndPosition()) {
                        if (cueMode == CUE_MODE_MIXXX) {
                            // in Mixxx mode Cue Button is flashing slow if CUE will move Cue point
                            m_pCueIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_500MS);
                        } else if (cueMode == CUE_MODE_MIXXX_NO_BLINK) {
                            m_pCueIndicator->setBlinkValue(ControlIndicator::OFF);
                        } else {
                            // in Pioneer mode Cue Button is flashing fast if CUE will move Cue point
                            m_pCueIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_250MS);
                        }
                    } else {
                        // At track end
                        m_pCueIndicator->setBlinkValue(ControlIndicator::OFF);
                    }
                } else if (m_pCuePoint->get() != -1) {
                    // Next Press is preview
                    m_pCueIndicator->setBlinkValue(ControlIndicator::ON);
                }
            }
        }
    }
}

bool CueControl::isTrackAtCue()
{
    return (std::abs(getCurrentSample() - m_pCuePoint->get()) < 1.0f);
}

ConfigKey HotcueControl::keyForControl(int hotcue, const char* name)
{
    // Add one to hotcue so that we dont have a hotcue_0
    return { m_group,QLatin1String("hotcue_") % QString::number(hotcue+1) % "_" % name};
}

HotcueControl::HotcueControl(QObject *pParent,QString group, int i)
        : QObject(pParent),
          m_group(group),
          m_iHotcueNumber(i),
          m_pCue(NULL),
          m_bPreviewing(false),
          m_iPreviewingPosition(-1)
{
    m_hotcuePosition = new ControlObject(keyForControl(i, "position"),this);
    connect(m_hotcuePosition, SIGNAL(valueChanged(double)),
            this, SLOT(slotHotcuePositionChanged(double)),Qt::AutoConnection);
    m_hotcuePosition->set(-1);

    m_hotcueEnabled = new ControlObject(keyForControl(i, "enabled"),this);
    m_hotcueEnabled->set(0);

    m_hotcueSet = new ControlPushButton(keyForControl(i, "set"),this);
    connect(m_hotcueSet, SIGNAL(valueChanged(double)),
            this, SLOT(slotHotcueSet(double)),Qt::AutoConnection);

    m_hotcueGoto = new ControlPushButton(keyForControl(i, "goto"),this);
    connect(m_hotcueGoto, SIGNAL(valueChanged(double)),
            this, SLOT(slotHotcueGoto(double)),Qt::AutoConnection);

    m_hotcueGotoAndPlay = new ControlPushButton(keyForControl(i, "gotoandplay"),this);
    connect(m_hotcueGotoAndPlay, SIGNAL(valueChanged(double)),
            this, SLOT(slotHotcueGotoAndPlay(double)),Qt::AutoConnection);

    m_hotcueGotoAndStop = new ControlPushButton(keyForControl(i, "gotoandstop"),this);
    connect(m_hotcueGotoAndStop, SIGNAL(valueChanged(double)),
            this, SLOT(slotHotcueGotoAndStop(double)),Qt::AutoConnection);

    m_hotcueActivate = new ControlPushButton(keyForControl(i, "activate"),this);
    connect(m_hotcueActivate, SIGNAL(valueChanged(double)),
            this, SLOT(slotHotcueActivate(double)),Qt::AutoConnection);

    m_hotcueActivatePreview = new ControlPushButton(keyForControl(i, "activate_preview"),this);
    connect(m_hotcueActivatePreview, SIGNAL(valueChanged(double)),
            this, SLOT(slotHotcueActivatePreview(double)),Qt::AutoConnection);

    m_hotcueClear = new ControlPushButton(keyForControl(i, "clear"),this);
    connect(m_hotcueClear, SIGNAL(valueChanged(double)),
            this, SLOT(slotHotcueClear(double)),Qt::AutoConnection);
}

HotcueControl::~HotcueControl() {
    delete m_hotcuePosition;
    delete m_hotcueEnabled;
    delete m_hotcueSet;
    delete m_hotcueGoto;
    delete m_hotcueGotoAndPlay;
    delete m_hotcueGotoAndStop;
    delete m_hotcueActivate;
    delete m_hotcueActivatePreview;
    delete m_hotcueClear;
}

void HotcueControl::slotHotcueSet(double v) {
    emit(hotcueSet(this, v));
}

void HotcueControl::slotHotcueGoto(double v) {
    emit(hotcueGoto(this, v));
}

void HotcueControl::slotHotcueGotoAndPlay(double v) {
    emit(hotcueGotoAndPlay(this, v));
}

void HotcueControl::slotHotcueGotoAndStop(double v) {
    emit(hotcueGotoAndStop(this, v));
}

void HotcueControl::slotHotcueActivate(double v) {
    emit(hotcueActivate(this, v));
}

void HotcueControl::slotHotcueActivatePreview(double v) {
    emit(hotcueActivatePreview(this, v));
}

void HotcueControl::slotHotcueClear(double v) {
    emit(hotcueClear(this, v));
}

void HotcueControl::slotHotcuePositionChanged(double newPosition) {
    emit(hotcuePositionChanged(this, newPosition));
}
