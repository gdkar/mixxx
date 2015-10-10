#include "engine/vinylcontrolcontrol.h"

#include "vinylcontrol/vinylcontrol.h"
#include "library/dao/cue.h"
#include "util/math.h"
#include "controlobjectslave.h"
#include "controlobject.h"
#include "controlpushbutton.h"
VinylControlControl::VinylControlControl(QString group, ConfigObject<ConfigValue>* pConfig,QObject *pParent)
        : EngineControl(group, pConfig,pParent)
{
    m_pControlVinylStatus = new ControlObject(ConfigKey(group, "vinylcontrol_status"),this);
    m_pControlVinylSpeedType = new ControlObject(ConfigKey(group, "vinylcontrol_speed_type"),this);
    //Convert the ConfigKey's value into a double for the CO (for fast reads).
    auto strVinylSpeedType = pConfig->getValueString(ConfigKey(group, "vinylcontrol_speed_type"));
    if (strVinylSpeedType == MIXXX_VINYL_SPEED_33) {
        m_pControlVinylSpeedType->set(MIXXX_VINYL_SPEED_33_NUM);
    } else if (strVinylSpeedType == MIXXX_VINYL_SPEED_45) {
        m_pControlVinylSpeedType->set(MIXXX_VINYL_SPEED_45_NUM);
    } else {
        m_pControlVinylSpeedType->set(MIXXX_VINYL_SPEED_33_NUM);
    }
    m_pControlVinylSeek = new ControlObject(ConfigKey(group, "vinylcontrol_seek"),this);
    connect(m_pControlVinylSeek, SIGNAL(valueChanged(double)),this, SLOT(slotControlVinylSeek(double)),Qt::DirectConnection);
    m_pControlVinylRate = new ControlObject(ConfigKey(group, "vinylcontrol_rate"),this);
    m_pControlVinylScratching = new ControlPushButton(ConfigKey(group, "vinylcontrol_scratching"),false,this);
    m_pControlVinylScratching->set(0);
    m_pControlVinylScratching->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylEnabled = new ControlPushButton(ConfigKey(group, "vinylcontrol_enabled"),false,this);
    m_pControlVinylEnabled->set(0);
    m_pControlVinylEnabled->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylWantEnabled = new ControlPushButton(ConfigKey(group, "vinylcontrol_wantenabled"),false,this);
    m_pControlVinylWantEnabled->set(0);
    m_pControlVinylWantEnabled->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylMode = new ControlPushButton(ConfigKey(group, "vinylcontrol_mode"),false,this);
    m_pControlVinylMode->setStates(3);
    m_pControlVinylMode->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylCueing = new ControlPushButton(ConfigKey(group, "vinylcontrol_cueing"),false,this);
    m_pControlVinylCueing->setStates(3);
    m_pControlVinylCueing->setButtonMode(ControlPushButton::TOGGLE);
    m_pControlVinylSignalEnabled = new ControlPushButton(ConfigKey(group, "vinylcontrol_signal_enabled"),false,this);
    m_pControlVinylSignalEnabled->set(1);
    m_pControlVinylSignalEnabled->setButtonMode(ControlPushButton::TOGGLE);
    m_pPlayEnabled = new ControlObjectSlave(group, "play", this);
}
VinylControlControl::~VinylControlControl() = default;
void VinylControlControl::trackLoaded(TrackPointer pTrack)
{
    m_pCurrentTrack = pTrack;
}
void VinylControlControl::trackUnloaded(TrackPointer /*pTrack*/)
{
    m_pCurrentTrack.clear();
}
void VinylControlControl::notifySeekQueued() {
    // m_bRequested is set and unset in a single execution path,
    // so there are no issues with signals/slots causing timing
    // issues.
    if (m_pControlVinylMode->get() == MIXXX_VCMODE_ABSOLUTE && m_pPlayEnabled->get() > 0.0 && !m_bSeekRequested)
        m_pControlVinylMode->set(MIXXX_VCMODE_RELATIVE);
}
void VinylControlControl::slotControlVinylSeek(double fractionalPos) {
    // Prevent NaN's from sneaking into the engine.
    if (isnan(fractionalPos)) {return;}
    // Do nothing if no track is loaded.
    if (!m_pCurrentTrack) {return;}
    auto total_samples = getTotalSamples();
    auto new_playpos = round(fractionalPos * total_samples);
    if (m_pControlVinylEnabled->get() > 0.0 && m_pControlVinylMode->get() == MIXXX_VCMODE_RELATIVE) {
        auto cuemode = static_cast<int>(m_pControlVinylCueing->get());
        //if in preroll, always seek
        if (new_playpos < 0) {
            seekExact(new_playpos);
            return;
        }
        switch (cuemode)
        {
          case MIXXX_RELATIVE_CUE_OFF:  return; // If off, do nothing.
          case MIXXX_RELATIVE_CUE_ONECUE:
                                      {
                                        //if onecue, just seek to the regular cue
                                         seekExact(m_pCurrentTrack->getCuePoint());
                                         return;
                                      }
              // Continue processing in this function.
          case MIXXX_RELATIVE_CUE_HOTCUE:break;
          default:
                                      {
                                        qWarning() << "Invalid vinyl cue setting";
                                        return;
                                      }
        }
        auto shortest_distance = 0.0;
        auto nearest_playpos = -1;
        auto cuePoints = m_pCurrentTrack->getCuePoints();
        for(auto pCue:cuePoints)
        {
            if (pCue->getType() != Cue::CUE || pCue->getHotCue() == -1) {continue;}
            auto cue_position = pCue->getPosition();
            //pick cues closest to new_playpos
            if ((nearest_playpos == -1) || (std::abs(new_playpos - cue_position) < shortest_distance))
            {
                nearest_playpos = cue_position;
                shortest_distance = std::abs(new_playpos - cue_position);
            }
        }

        if (nearest_playpos == -1)
        {
            if (new_playpos >= 0)return;
            //if negative, allow a seek by falling down to the bottom
        }
        else
        {
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
