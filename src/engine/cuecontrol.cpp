// cuecontrol.cpp
// Created 11/5/2009 by RJ Ryan (rryan@mit.edu)

#include <QMutexLocker>

#include "engine/cuecontrol.h"

#include "controlobject.h"
#include "controlpushbutton.h"
#include "controlindicator.h"
#include "trackinfoobject.h"
#include "library/dao/cue.h"
#include "engine/cachingreader.h"
#include "vinylcontrol/defs_vinylcontrol.h"

static const double CUE_MODE_MIXXX = 0.0;
static const double CUE_MODE_PIONEER = 1.0;
static const double CUE_MODE_DENON = 2.0;
static const double CUE_MODE_NUMARK = 3.0;

CueControl::CueControl(QString group,
                       ConfigObject<ConfigValue>* _config, QObject *pParent) :
        EngineControl(group, _config, pParent),
        m_bHotcueCancel(false),
        m_bPreviewing(false),
        m_bPreviewingHotcue(false),
        m_pPlayButton(ControlObject::getControl(ConfigKey(group, "play"))),
        m_pStopButton(ControlObject::getControl(ConfigKey(group, "stop"))),
        m_iCurrentlyPreviewingHotcues(0),
        m_iNumHotCues(NUM_HOT_CUES),
        m_pLoadedTrack(),
        m_mutex(QMutex::Recursive) {
    // To silence a compiler warning about CUE_MODE_PIONEER.
    Q_UNUSED(CUE_MODE_PIONEER);
    createControls();
    m_pTrackSamples = ControlObject::getControl(ConfigKey(group, "track_samples"));

    m_pQuantizeEnabled = ControlObject::getControl(ConfigKey(group, "quantize"));

    m_pNextBeat = ControlObject::getControl(ConfigKey(group, "beat_next"));
    m_pClosestBeat = ControlObject::getControl(ConfigKey(group, "beat_closest"));

    m_pCuePoint = new ControlObject(ConfigKey(group, "cue_point"));
    m_pCuePoint->set(-1.0);

    m_pCueMode = new ControlObject(ConfigKey(group, "cue_mode"));
    // 0.0 -> Pioneer mode
    // 1.0 -> Denon mode

    m_pCueSet = new ControlPushButton(ConfigKey(group, "cue_set"));
    m_pCueSet->setButtonMode(ControlPushButton::TRIGGER);
    connect(m_pCueSet, &ControlObject::valueChanged,
            this, &CueControl::cueSet,
            Qt::DirectConnection);
    m_pCueGoto = new ControlPushButton(ConfigKey(group, "cue_goto"));
    connect(m_pCueGoto, &ControlObject::valueChanged,
            this, &CueControl::cueGoto,
            Qt::DirectConnection);
    m_pCueGotoAndPlay = new ControlPushButton(ConfigKey(group, "cue_gotoandplay"));
    connect(m_pCueGotoAndPlay, &ControlObject::valueChanged,
            this, &CueControl::cueGotoAndPlay,
            Qt::DirectConnection);
    m_pCueGotoAndStop = new ControlPushButton(ConfigKey(group, "cue_gotoandstop"));
    connect(m_pCueGotoAndStop, &ControlObject::valueChanged,
            this, &CueControl::cueGotoAndStop,
            Qt::DirectConnection);
    m_pCuePreview = new ControlPushButton(ConfigKey(group, "cue_preview"));
    connect(m_pCuePreview, &ControlObject::valueChanged,
            this, &CueControl::cuePreview,
            Qt::DirectConnection);

    m_pCueCDJ = new ControlPushButton(ConfigKey(group, "cue_cdj"));
    connect(m_pCueCDJ, &ControlObject::valueChanged,
            this, &CueControl::cueCDJ,
            Qt::DirectConnection);

    m_pCueDefault = new ControlPushButton(ConfigKey(group, "cue_default"));
    connect(m_pCueDefault, &ControlObject::valueChanged,
            this, &CueControl::cueDefault,
            Qt::DirectConnection);
    m_pPlayStutter = new ControlPushButton(ConfigKey(group, "play_stutter"));
    connect(m_pPlayStutter, &ControlObject::valueChanged,
            this, &CueControl::playStutter,
            Qt::DirectConnection);
    m_pCueIndicator = new ControlIndicator(ConfigKey(group, "cue_indicator"));
    m_pPlayIndicator = new ControlIndicator(ConfigKey(group, "play_indicator"));
    m_pVinylControlEnabled = new ControlObjectSlave(group, "vinylcontrol_enabled");
    m_pVinylControlMode = new ControlObjectSlave(group, "vinylcontrol_mode");
}
CueControl::~CueControl() {
    delete m_pCuePoint;
    delete m_pCueMode;
    delete m_pCueSet;
    delete m_pCueGoto;
    delete m_pCueGotoAndPlay;
    delete m_pCueGotoAndStop;
    delete m_pCuePreview;
    delete m_pCueCDJ;
    delete m_pCueDefault;
    delete m_pPlayStutter;
    delete m_pCueIndicator;
    delete m_pPlayIndicator;
    delete m_pVinylControlEnabled;
    delete m_pVinylControlMode;
    qDeleteAll(m_hotcueControl);
}
void CueControl::createControls() {
    for (int i = 0; i < m_iNumHotCues; ++i) {
        auto pControl = new HotcueControl(getGroup(), i, this);
        connect(pControl, &HotcueControl::hotcuePositionChanged,
                this, &CueControl::hotcuePositionChanged,
                Qt::DirectConnection);
        connect(pControl, &HotcueControl::hotcueSet,
                this, &CueControl::hotcueSet,
                Qt::DirectConnection);
        connect(pControl, &HotcueControl::hotcueGoto,
                this, &CueControl::hotcueGoto,
                Qt::DirectConnection);
        connect(pControl, &HotcueControl::hotcueGotoAndPlay,
                this, &CueControl::hotcueGotoAndPlay,
                Qt::DirectConnection);
        connect(pControl, &HotcueControl::hotcueGotoAndStop,
                this, &CueControl::hotcueGotoAndStop,
                Qt::DirectConnection);
        connect(pControl, &HotcueControl::hotcueActivate,
                this, &CueControl::hotcueActivate,
                Qt::DirectConnection);
        connect(pControl, &HotcueControl::hotcueActivatePreview,
                this, &CueControl::hotcueActivatePreview,
                Qt::DirectConnection);
        connect(pControl, &HotcueControl::hotcueClear,
                this, &CueControl::hotcueClear,
                Qt::DirectConnection);
        m_hotcueControl.append(pControl);
    }
}
void CueControl::attachCue(QSharedPointer<Cue> pCue, int hotCue) {
    auto pControl = m_hotcueControl.value(hotCue, nullptr);
    if (pControl == nullptr) {return;}
    if (pControl->getCue() != nullptr) {detachCue(pControl->getHotcueNumber());}
    connect(pCue.data(), &Cue::updated,this, &CueControl::cueUpdated,Qt::DirectConnection);
    pControl->getPosition()->set(pCue->getPosition());
    pControl->getEnabled()->set(pCue->getPosition() == -1 ? 0.0 : 1.0);
    // set pCue only if all other data is in place
    // because we have a null check for valid data else where  in the code
    pControl->setCue(pCue);

}
void CueControl::detachCue(int hotCue) {
    auto pControl = m_hotcueControl.value(hotCue, nullptr);
    if (pControl == nullptr) {return;}
    auto pCue = pControl->getCue();
    if (!pCue) return;
    disconnect(pCue.data(), 0, this, 0);
    // clear pCue first because we have a null check for valid data else where
    // in the code
    pControl->clearCue();
    pControl->getPosition()->set(-1); // invalidate position for hintReader()
    pControl->getEnabled()->set(0);
}
void CueControl::trackLoaded(TrackPointer pTrack) {
    QMutexLocker lock(&m_mutex);
    if (m_pLoadedTrack) trackUnloaded(m_pLoadedTrack);
    if (!pTrack) {return;}
    m_pLoadedTrack = pTrack;
    connect(pTrack.data(), &TrackInfoObject::cuesUpdated,this, &CueControl::trackCuesUpdated,static_cast<Qt::ConnectionType>(Qt::QueuedConnection|Qt::UniqueConnection));
    auto loadCue = QSharedPointer<Cue>{};
    const auto cuePoints = pTrack->getCuePoints();
    for(auto &pCue : cuePoints){
        if (pCue->getType() == Cue::LOAD) {
            loadCue = pCue;
        } else if (pCue->getType() != Cue::CUE) {continue;}
        auto hotcue = pCue->getHotCue();
        if (hotcue != -1) attachCue(pCue, hotcue);
    }
    auto loadCuePoint = 0.0;
    // If cue recall is ON in the prefs, then we're supposed to seek to the cue
    // point on song load. Note that [Controls],cueRecall == 0 corresponds to "ON", not OFF.
    auto cueRecall = (getConfig()->getValueString(ConfigKey("[Controls]","CueRecall"), "0").toInt() == 0);
    if (!loadCue.isNull()) {
        m_pCuePoint->set(loadCue->getPosition());
        if (cueRecall) {loadCuePoint = loadCue->getPosition();}
    } else {
        // If no cue point is stored, set one at track start
        m_pCuePoint->set(0.0);
    }
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    // If cueRecall is on, seek to it even if we didn't find a cue value (we'll
    // seek to 0.
    if (cueRecall) { seekExact(loadCuePoint);}
     else if (!(m_pVinylControlEnabled->get() &&
            m_pVinylControlMode->get() == MIXXX_VCMODE_ABSOLUTE)) {
        // If cuerecall is off, seek to zero unless
        // vinylcontrol is on and set to absolute.  This allows users to
        // load tracks and have the needle-drop be maintained.
        seekExact(0.0);
    }
}
void CueControl::trackUnloaded(TrackPointer pTrack) {
    QMutexLocker lock(&m_mutex);
    disconnect(pTrack.data(), 0, this, 0);
    for (int i = 0; i < m_iNumHotCues; ++i) {detachCue(i);}
    // Store the cue point in a load cue.
    auto cuePoint = m_pCuePoint->get();
    if (cuePoint != -1 && cuePoint != 0.0) {
        auto loadCue = QSharedPointer<Cue>{};
        const auto cuePoints = pTrack->getCuePoints();
        for(auto &pCue : cuePoints){
            if (pCue->getType() == Cue::LOAD) {
                loadCue = pCue;
                break;
            }
        }
        if (loadCue.isNull()) {
            loadCue = pTrack->addCue();
            loadCue->setType(Cue::LOAD);
            loadCue->setLength(0);
        }
        loadCue->setPosition(cuePoint);
    }
    m_pCueIndicator->setBlinkValue(ControlIndicator::OFF);
    m_pCuePoint->set(-1.0);
    m_pLoadedTrack.clear();
}
void CueControl::cueUpdated() {
    //QMutexLocker lock(&m_mutex);
    // We should get a trackCuesUpdated call anyway, so do nothing.
}
void CueControl::trackCuesUpdated() {
    QMutexLocker lock(&m_mutex);
    QSet<int> active_hotcues;
    if (!m_pLoadedTrack) return;
    const auto cuePoints = m_pLoadedTrack->getCuePoints();
    for(auto &pCue : cuePoints){
        if (pCue->getType() != Cue::CUE && pCue->getType() != Cue::LOAD) continue;
        int hotcue = pCue->getHotCue();
        if (hotcue != -1) {
            HotcueControl* pControl = m_hotcueControl.value(hotcue, nullptr);
            // Cue's hotcue doesn't have a hotcue control.
            if (pControl == nullptr) {continue;}
            auto pOldCue = pControl->getCue();
            // If the old hotcue is different than this one.
            if (pOldCue != pCue) {
                // If the old hotcue exists, detach it
                if (!pOldCue.isNull()) detachCue(hotcue);
                attachCue(pCue, hotcue);
            } else {
                // If the old hotcue is the same, then we only need to update
                auto dOldPosition = pControl->getPosition()->get();
                auto dOldEnabled = pControl->getEnabled()->get();
                auto dPosition = pCue->getPosition();
                auto  dEnabled = dPosition == -1 ? 0.0 : 1.0;
                if (dEnabled != dOldEnabled) {pControl->getEnabled()->set(dEnabled);}
                if (dPosition != dOldPosition) {pControl->getPosition()->set(dPosition);}
            }
            // Add the hotcue to the list of active hotcues
            active_hotcues.insert(hotcue);
        }
    }
    // Detach all hotcues that are no longer present
    for (auto i = 0; i < m_iNumHotCues; ++i) {
        if (!active_hotcues.contains(i))detachCue(i);
    }
}
void CueControl::hotcueSet(double v) {
    HotcueControl *pControl = qobject_cast<HotcueControl*>(sender());
    //qDebug() << "CueControl::hotcueSet" << v;
    if (!v)return;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack)return;
    auto  hotcue = pControl->getHotcueNumber();
    detachCue(hotcue);
    auto pCue = m_pLoadedTrack->addCue();
    auto cuePosition = (m_pQuantizeEnabled->get() > 0.0 && m_pClosestBeat->get() != -1) ?
                          floor(m_pClosestBeat->get()) : floor(getCurrentSample());
    if (!even(static_cast<int>(cuePosition)))cuePosition--;
    pCue->setPosition(cuePosition);
    pCue->setHotCue(hotcue);
    pCue->setLabel("");
    pCue->setType(Cue::CUE);
    // TODO(XXX) deal with spurious signals
    attachCue(pCue, hotcue);
    // If quantize is enabled and we are not playing, jump to the cue point
    // since it's not necessarily where we currently are. TODO(XXX) is this
    // potentially invalid for vinyl control?
    auto playing = m_pPlayButton->get() > 0;
    if (!playing && m_pQuantizeEnabled->get() > 0.0) {
        lock.unlock();  // prevent deadlock.
        // Enginebuffer will quantize more exactly than we can.
        seekAbs(cuePosition);
    }
}
void CueControl::hotcueGoto(double v) {
    HotcueControl *pControl = qobject_cast<HotcueControl*>(sender());
    if (!v)return;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack) {return;}
    auto pCue = pControl->getCue();
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    if (pCue) {
        auto position = pCue->getPosition();
        if (position != -1) {seekAbs(position);}
    }
}
void CueControl::hotcueGotoAndStop(double v) {
    auto pControl = qobject_cast<HotcueControl*>(sender());
    if (!v)return;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack)return;
    auto pCue = pControl->getCue();
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    if (!pCue.isNull()) {
        auto position = pCue->getPosition();
        if (position != -1) {
            m_pPlayButton->set(0.0);
            seekExact(position);
        }
    }
}
void CueControl::hotcueGotoAndPlay( double v) {
    auto pControl = qobject_cast<HotcueControl*>(sender());
    if (!v) return;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack) {return;}
    auto pCue = pControl->getCue();
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    if (pCue) {
        int position = pCue->getPosition();
        if (position != -1) {
            seekAbs(position);
            m_pPlayButton->set(1.0);
        }
    }
}
void CueControl::hotcueActivate(double v) {
    auto pControl = qobject_cast<HotcueControl*>(sender());
    //qDebug() << "CueControl::hotcueActivate" << v;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack) {return;}
    auto pCue = pControl->getCue();
    lock.unlock();
    if (!pCue.isNull()) {
        if (v) {
            m_bHotcueCancel = false;
            if (pCue->getPosition() == -1) {hotcueSet(v);}
            else {
                if (!m_bPreviewingHotcue && m_pPlayButton->get() == 1.0) {hotcueGoto( v);}
                else {hotcueActivatePreview( v);}
            }
        } else {if (pCue->getPosition() != -1) {hotcueActivatePreview( v);}}
    } else {
        if (v) {
            // just in case
            m_bHotcueCancel = false;
            hotcueSet(v);
        } else if (m_bPreviewingHotcue) {
            // The cue is non-existent, yet we got a release for it and are
            // currently previewing a hotcue. This is indicative of a corner
            // case where the cue was detached while we were pressing it. Let
            // hotcueActivatePreview handle it.
            hotcueActivatePreview(v);
        }
    }
}
void CueControl::hotcueActivatePreview(double v) {
    auto pControl = qobject_cast<HotcueControl*>(sender());
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack) {return;}
    auto pCue = pControl->getCue();
    if (v) {
        if (pCue && pCue->getPosition() != -1) {
            m_iCurrentlyPreviewingHotcues++;
            auto iPosition = pCue->getPosition();
            m_bPreviewingHotcue = true;
            m_pPlayButton->set(1.0);
            pControl->setPreviewing(true);
            pControl->setPreviewingPosition(iPosition);
            // Need to unlock before emitting any signals to prevent deadlock.
            lock.unlock();
            seekAbs(iPosition);
        }
    } else if (m_bPreviewingHotcue) {
        // This is a activate release and we are previewing at least one
        // hotcue. If this hotcue is previewing:
        if (pControl->isPreviewing()) {
            // Mark this hotcue as not previewing.
            auto iPosition = pControl->getPreviewingPosition();
            pControl->setPreviewing(false);
            pControl->setPreviewingPosition(-1);
            // If this is the last hotcue to leave preview.
            if (--m_iCurrentlyPreviewingHotcues == 0) {
                bool bHotcueCancel = m_bHotcueCancel;
                m_bPreviewingHotcue = false;
                m_bHotcueCancel = false;
                // If hotcue cancel was not marked then snap back to the
                // hotcue and stop.
                if (!bHotcueCancel) {
                    m_pPlayButton->set(0.0);
                    // Need to unlock before emitting any signals to prevent deadlock.
                    lock.unlock();
                    seekExact(iPosition);
                }
            }
        }
    }
}

void CueControl::hotcueClear(double v) {
    auto pControl = qobject_cast<HotcueControl*>(sender());
    if (!v) return;
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack) { return;}
    auto pCue = pControl->getCue();
    if (pCue) {pCue->setHotCue(-1);}
    detachCue(pControl->getHotcueNumber());
}
void CueControl::hotcuePositionChanged(double newPosition) {
    auto pControl = qobject_cast<HotcueControl*>(sender());
    QMutexLocker lock(&m_mutex);
    if (!m_pLoadedTrack) return;
    auto pCue = pControl->getCue();
    if (!pCue.isNull()) {
        // Setting the position to -1 is the same as calling hotcue_x_clear
        if (newPosition == -1) {
            pCue->setHotCue(-1);
            detachCue(pControl->getHotcueNumber());
        } else if (newPosition > 0 && newPosition < m_pTrackSamples->get()) {
            auto position = static_cast<size_t>(newPosition);
            // People writing from MIDI land, elsewhere might be careless.
            if (position % 2 != 0) {position--;}
            pCue->setPosition(position);
        }
    }
}
void CueControl::hintReader(HintVector* pHintList) {
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
    for(auto &pControl : m_hotcueControl){
        auto position = pControl->getPosition()->get();
        if (position != -1) {
            cue_hint.sample = position;
            if (cue_hint.sample % 2 != 0) cue_hint.sample--;
            cue_hint.length = 0;
            cue_hint.priority = 10;
            pHintList->append(cue_hint);
        }
    }
}
void CueControl::saveCuePoint(double cuePoint) {if (m_pLoadedTrack) {m_pLoadedTrack->setCuePoint(cuePoint);}}
void CueControl::cueSet(double v) {
    if (!v)return;
    QMutexLocker lock(&m_mutex);
    auto cue = (m_pQuantizeEnabled->get() > 0.0 && m_pClosestBeat->get() != -1) ? floor(m_pClosestBeat->get()) : floor(getCurrentSample());
    if (!even(static_cast<int>(cue))) cue--;
    m_pCuePoint->set(cue);
    saveCuePoint(cue);
}
void CueControl::cueGoto(double v){
    if (!v) return;
    QMutexLocker lock(&m_mutex);
    // Seek to cue point
    auto cuePoint = m_pCuePoint->get();
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    seekAbs(cuePoint);
}
void CueControl::cueGotoAndPlay(double v){
    if (!v) return;
    cueGoto(v);
    QMutexLocker lock(&m_mutex);
    // Start playing if not already
    if (m_pPlayButton->get()==0.) {m_pPlayButton->set(1.0);}
}
void CueControl::cueGotoAndStop(double v){
    if (!v) return;
    QMutexLocker lock(&m_mutex);
    m_pPlayButton->set(0.0);
    auto cuePoint = m_pCuePoint->get();
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    seekExact(cuePoint);
}
void CueControl::cuePreview(double v){
    QMutexLocker lock(&m_mutex);
    if (v) {
        m_bPreviewing = true;
        m_pPlayButton->set(1.0);
    } else if (!v && m_bPreviewing) {
        m_bPreviewing = false;
        m_pPlayButton->set(0.0);
    }
    auto cuePoint = m_pCuePoint->get();
    // Need to unlock before emitting any signals to prevent deadlock.
    lock.unlock();
    seekAbs(cuePoint);
}
void CueControl::cueCDJ(double v) {
    // This is how Pioneer cue buttons work:
    // If pressed while playing, stop playback and go to cue.
    // If pressed while stopped and at cue, play while pressed.
    // If pressed while stopped and not at cue, set new cue point.
    // If play is pressed while holding cue, the deck is now playing. (Handled in playFromCuePreview().)
    QMutexLocker lock(&m_mutex);
    auto playing = (m_pPlayButton->get() == 1.0);
    if (v) {
        if (playing || atEndPosition()) {
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
        m_pPlayButton->set(0.0);

        // Need to unlock before emitting any signals to prevent deadlock.
        lock.unlock();
        seekAbs(m_pCuePoint->get());
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
    auto playing = (m_pPlayButton->get() == 1.0);
    if (v) {
        if (!playing && isTrackAtCue()) {
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
        m_pPlayButton->set(0.0);
        // Need to unlock before emitting any signals to prevent deadlock.
        lock.unlock();
        seekAbs(m_pCuePoint->get());
    }
}

void CueControl::cueDefault(double v) {
    auto cueMode = m_pCueMode->get();
    // Decide which cue implementation to call based on the user preference
    if (cueMode == CUE_MODE_DENON || cueMode == CUE_MODE_NUMARK) {
        cueDenon(v);
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
    if (v != 0.0) {m_pPlayButton->set(0.0);}
}
void CueControl::playStutter(double v) {
    QMutexLocker lock(&m_mutex);
    //qDebug() << "playStutter" << v;
    if (v != 0.0) {
        if (m_pPlayButton->get() != 0.0) {cueGoto(1.0);}
        else {m_pPlayButton->set(1.0);}
    }
}
double CueControl::updateIndicatorsAndModifyPlay(double play, bool playPossible) {
    QMutexLocker lock(&m_mutex);
    auto cueMode = m_pCueMode->get();
    if ((cueMode == CUE_MODE_DENON || cueMode == CUE_MODE_NUMARK) &&
            play > 0.0 &&
            playPossible &&
            m_pPlayButton->get() == 0.0) {
        // in Denon mode each play from pause moves the cue point
        // if not previewing
        cueSet(1.0);
    }
    // when previewing, "play" was set by cue button, a following toggle request
    // (play = 0.0) is used for latching play.
    auto previewing = false;
    if (m_bPreviewing || m_bPreviewingHotcue) {
        if (play == 0.0) {
            // play latch request: stop previewing and go into normal play mode.
            m_bPreviewing = false;
            m_bHotcueCancel = true;
            play = 1.0;
        } else {
            previewing = true;
        }
    }
    if (!playPossible) {
        // play not possible
        play = 0.0;
        m_pPlayIndicator->setBlinkValue(ControlIndicator::OFF);
        m_pStopButton->set(0.0);
    } else if (play && !previewing) {
        // Play: Indicates a latched Play
        m_pPlayIndicator->setBlinkValue(ControlIndicator::ON);
        m_pStopButton->set(0.0);
    } else {
        // Pause:
        m_pStopButton->set(1.0);
        if (cueMode == CUE_MODE_DENON) {
            if (isTrackAtCue()) {
                m_pPlayIndicator->setBlinkValue(ControlIndicator::OFF);
            } else {
                // Flashing indicates that a following play would move cue point
                m_pPlayIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_500MS);
            }
        } else if (cueMode == CUE_MODE_MIXXX || cueMode == CUE_MODE_NUMARK) {
            m_pPlayIndicator->setBlinkValue(ControlIndicator::OFF);
        } else {
            // Flashing indicates that play is possible in Pioneer mode
            m_pPlayIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_500MS);
        }
    }

    if (cueMode != CUE_MODE_DENON && cueMode != CUE_MODE_NUMARK) {
        if (m_pCuePoint->get() != -1) {
            if (play == 0.0 && !isTrackAtCue() &&
                    !atEndPosition()) {
                if (cueMode == CUE_MODE_MIXXX) {
                    // in Mixxx mode Cue Button is flashing slow if CUE will move Cue point
                    m_pCueIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_500MS);
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
    m_pPlayStutter->set(play);
    return play;
}
void CueControl::updateIndicators() {
    // No need for mutex lock because we are only touching COs.
    auto cueMode = m_pCueMode->get();
    if (cueMode == CUE_MODE_DENON || cueMode == CUE_MODE_NUMARK) {
        // Cue button is only lit at cue point
        auto playing = m_pPlayButton->get() > 0;
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
            auto playing = m_pPlayButton->get() > 0;
            if (!playing) {
                if (!isTrackAtCue()) {
                    if (!atEndPosition()) {
                        if (cueMode == CUE_MODE_MIXXX) {
                            // in Mixxx mode Cue Button is flashing slow if CUE will move Cue point
                            m_pCueIndicator->setBlinkValue(ControlIndicator::RATIO1TO1_500MS);
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
bool CueControl::isTrackAtCue(){ return (fabs(getCurrentSample() - m_pCuePoint->get()) < 1.0f);}
ConfigKey HotcueControl::keyForControl(int hotcue, QString name) {
    ConfigKey key;
    key.group = m_group;
    // Add one to hotcue so that we dont have a hotcue_0
    key.item = QString("hotcue_%1_%2").arg(QString::number(hotcue+1), name);
    return key;
}
HotcueControl::HotcueControl(QString group, int i, QObject *pParent)
        : QObject(pParent),
          m_group(group),
          m_iHotcueNumber(i),
          m_pCue(nullptr),
          m_hotcuePosition(keyForControl(i,"position")),
          m_hotcueEnabled(keyForControl(i,"enabled")),
          m_hotcueSet(keyForControl(i,"set")),
          m_hotcueGoto(keyForControl(i,"goto")),
          m_hotcueGotoAndPlay(keyForControl(i,"gotoandplay")),
          m_hotcueGotoAndStop(keyForControl(i,"gotoandstop")),
          m_hotcueActivate(keyForControl(i,"activate")),
          m_hotcueActivatePreview(keyForControl(i,"activatePreview")),
          m_hotcueClear(keyForControl(i,"clear")),
          m_bPreviewing(false),
          m_iPreviewingPosition(-1) {
    connect(&m_hotcuePosition, &ControlObject::valueChanged,
            this, &HotcueControl::hotcuePositionChanged,
            Qt::DirectConnection);
    m_hotcuePosition.set(-1);
    m_hotcueEnabled.set(0);
    connect(&m_hotcueSet, &ControlObject::valueChanged,
            this, &HotcueControl::hotcueSet,
            Qt::DirectConnection);
    connect(&m_hotcueGoto, &ControlObject::valueChanged,
            this, &HotcueControl::hotcueGoto,
            Qt::DirectConnection);
    connect(&m_hotcueGotoAndPlay, &ControlObject::valueChanged,
            this, &HotcueControl::hotcueGotoAndPlay,
            Qt::DirectConnection);
    connect(&m_hotcueGotoAndStop, &ControlObject::valueChanged,
            this, &HotcueControl::hotcueGotoAndStop,
            Qt::DirectConnection);
    connect(&m_hotcueActivate, &ControlObject::valueChanged,
            this, &HotcueControl::hotcueActivate,
            Qt::DirectConnection);
    connect(&m_hotcueActivatePreview, &ControlObject::valueChanged,
            this, &HotcueControl::hotcueActivatePreview,
            Qt::DirectConnection);
    connect(&m_hotcueClear, &ControlObject::valueChanged,
            this, &HotcueControl::hotcueClear,
            Qt::DirectConnection);
}
HotcueControl::~HotcueControl() {}
