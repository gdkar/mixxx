#include "engine/vinylcontrolcontrol.h"
#include "control/controlpushbutton.h"
#include "vinylcontrol/vinylcontrol.h"
#include "library/dao/cue.h"
#include "util/math.h"

VinylControlControl::VinylControlControl(QString group, UserSettingsPointer pConfig)
        : EngineControl(group, pConfig),
          m_bSeekRequested(false) {
    m_pControlVinylStatus = new ControlObject(ConfigKey(group, "vinylcontrol_status"),this);
    m_pControlVinylSpeedType = new ControlObject(ConfigKey(group, "vinylcontrol_speed_type"),this);

    //Convert the ConfigKey's value into a double for the CO (for fast reads).
    auto strVinylSpeedType = pConfig->getValueString(ConfigKey(group,
                                                      "vinylcontrol_speed_type"));
    if (strVinylSpeedType == MIXXX_VINYL_SPEED_33) {
        m_pControlVinylSpeedType->set(MIXXX_VINYL_SPEED_33_NUM);
    } else if (strVinylSpeedType == MIXXX_VINYL_SPEED_45) {
        m_pControlVinylSpeedType->set(MIXXX_VINYL_SPEED_45_NUM);
    } else {
        m_pControlVinylSpeedType->set(MIXXX_VINYL_SPEED_33_NUM);
    }

    m_pControlVinylSeek = new ControlObject(ConfigKey(group, "vinylcontrol_seek"),this);
    connect(m_pControlVinylSeek, SIGNAL(valueChanged(double)),
            this, SLOT(slotControlVinylSeek(double)),
            Qt::DirectConnection);

    m_pControlVinylRate = new ControlObject(ConfigKey(group, "vinylcontrol_rate"),this);
    auto button = new ControlPushButton(ConfigKey(group, "vinylcontrol_scratching"),this);
    m_pControlVinylScratching = button;
    button->set(0);
    button->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylEnabled = button = new ControlPushButton(ConfigKey(group, "vinylcontrol_enabled"),this);
    button->set(0);
    button->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylWantEnabled = button = new ControlPushButton(ConfigKey(group, "vinylcontrol_wantenabled"),this);
    button->set(0);
    button->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylMode = button = new ControlPushButton(ConfigKey(group, "vinylcontrol_mode"),this);
    button ->setStates(3);
    button ->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylCueing = button = new ControlPushButton(ConfigKey(group, "vinylcontrol_cueing"),this);
    button->setStates(3);
    button->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylSignalEnabled = button = new ControlPushButton(ConfigKey(group, "vinylcontrol_signal_enabled"),this);
    button->set(1);
    button->setButtonMode(ControlPushButton::TOGGLE);

    m_pPlayEnabled = new ControlProxy(group, "play", this);
}

VinylControlControl::~VinylControlControl() {
    delete m_pControlVinylRate;
    delete m_pControlVinylSignalEnabled;
    delete m_pControlVinylCueing;
    delete m_pControlVinylMode;
    delete m_pControlVinylWantEnabled;
    delete m_pControlVinylEnabled;
    delete m_pControlVinylScratching;
    delete m_pControlVinylSeek;
    delete m_pControlVinylSpeedType;
    delete m_pControlVinylStatus;
}

void VinylControlControl::trackLoaded(TrackPointer pNewTrack, TrackPointer pOldTrack)
{
    Q_UNUSED(pOldTrack);
    m_pCurrentTrack = pNewTrack;
}

void VinylControlControl::notifySeekQueued()
{
    // m_bRequested is set and unset in a single execution path,
    // so there are no issues with signals/slots causing timing
    // issues.
    if (m_pControlVinylMode->get() == MIXXX_VCMODE_ABSOLUTE &&
        m_pPlayEnabled->get() > 0.0 &&
        !m_bSeekRequested) {
        m_pControlVinylMode->set(MIXXX_VCMODE_RELATIVE);
    }
}

void VinylControlControl::slotControlVinylSeek(double fractionalPos)
{
    // Prevent NaN's from sneaking into the engine.
    if (isnan(fractionalPos)) {
        return;
    }
    // Do nothing if no track is loaded.
    if (!m_pCurrentTrack) {
        return;
    }
    auto total_samples = getTotalSamples();
    auto new_playpos = round(fractionalPos * total_samples);

    if (m_pControlVinylEnabled->get() > 0.0 && m_pControlVinylMode->get() == MIXXX_VCMODE_RELATIVE) {
        auto cuemode = (int)m_pControlVinylCueing->get();
        //if in preroll, always seek
        if (new_playpos < 0) {
            seekExact(new_playpos);
            return;
        }
        switch (cuemode) {
        case MIXXX_RELATIVE_CUE_OFF:
            return; // If off, do nothing.
        case MIXXX_RELATIVE_CUE_ONECUE:
            //if onecue, just seek to the regular cue
            seekExact(m_pCurrentTrack->getCuePoint());
            return;
        case MIXXX_RELATIVE_CUE_HOTCUE:
            // Continue processing in this function.
            break;
        default:
            qWarning() << "Invalid vinyl cue setting";
            return;
        }

        auto shortest_distance = 0;
        auto nearest_playpos = -1;

        auto cuePoints = m_pCurrentTrack->getCuePoints();
        QListIterator<CuePointer> it(cuePoints);
        while (it.hasNext()) {
            CuePointer pCue(it.next());
            if (pCue->getType() != Cue::CUE || pCue->getHotCue() == -1) {
                continue;
            }
            auto cue_position = pCue->getPosition();
            //pick cues closest to new_playpos
            if ((nearest_playpos == -1) ||
                (std::abs(new_playpos - cue_position) < shortest_distance)) {
                nearest_playpos = cue_position;
                shortest_distance = fabs(new_playpos - cue_position);
            }
        }

        if (nearest_playpos == -1) {
            if (new_playpos >= 0) {
                //never found an appropriate cue, so don't seek?
                return;
            }
            //if negative, allow a seek by falling down to the bottom
        } else {
            m_bSeekRequested = true;
            seekExact(nearest_playpos);
            m_bSeekRequested = false;
            return;
        }
    }

    // Just seek where it wanted to originally.
    m_bSeekRequested = true;
    seekExact(new_playpos);
    m_bSeekRequested = false;
}

bool VinylControlControl::isEnabled()
{
    return m_pControlVinylEnabled->get();
}

bool VinylControlControl::isScratching()
{
    return m_pControlVinylScratching->get();
}
