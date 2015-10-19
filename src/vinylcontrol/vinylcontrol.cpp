#include "vinylcontrol/vinylcontrol.h"
#include "controlobject.h"

VinylControl::VinylControl(ConfigObject<ConfigValue> * pConfig, QString group)
        : m_pConfig(pConfig),
          m_group(group),
          m_iLeadInTime(m_pConfig->getValueString(
              ConfigKey(group, "vinylcontrol_lead_in_time")).toInt()),
          m_dVinylPosition(0.0),
          m_fTimecodeQuality(0.0f) {
    // Get Control objects
    m_pVinylControlInputGain = new ControlObject(ConfigKey(VINYL_PREF_KEY, "gain"),this);

    bool gainOk = false;
    double gain = m_pConfig->getValueString(ConfigKey(VINYL_PREF_KEY, "gain"))
            .toDouble(&gainOk);
    m_pVinylControlInputGain->set(gainOk ? gain : 1.0);

    playPos             = new ControlObject(ConfigKey(group, "playposition",this);    // Range: 0 to 1.0
    trackSamples        = new ControlObject(ConfigKey(group, "track_samples",this);
    trackSampleRate     = new ControlObject(ConfigKey(group, "track_samplerate",this);
    vinylSeek           = new ControlObject(ConfigKey(group, "vinylcontrol_seek",this);
    m_pVCRate = new ControlObject(ConfigKey(group, "vinylcontrol_rate",this);
    m_pRateSlider = new ControlObject(ConfigKey(group, "rate",this);
    playButton          = new ControlObject(ConfigKey(group, "play",this);
    duration            = new ControlObject(ConfigKey(group, "duration",this);
    mode                = new ControlObject(ConfigKey(group, "vinylcontrol_mode",this);
    enabled             = new ControlObject(ConfigKey(group, "vinylcontrol_enabled",this);
    wantenabled         = new ControlObject(ConfigKey(group, "vinylcontrol_wantenabled",this);
    cueing              = new ControlObject(ConfigKey(group, "vinylcontrol_cueing",this);
    scratching          = new ControlObject(ConfigKey(group, "vinylcontrol_scratching",this);
    rateRange           = new ControlObject(ConfigKey(group, "rateRange",this);
    vinylStatus         = new ControlObject(ConfigKey(group, "vinylcontrol_status",this);
    rateDir             = new ControlObject(ConfigKey(group, "rate_dir"),this);
    loopEnabled         = new ControlObject(ConfigKey(group, "loop_enabled"),this);
    signalenabled       = new ControlObject(ConfigKey(group, "vinylcontrol_signal_enabled"),this);
    reverseButton       = new ControlObject(ConfigKey(group, "reverse"),this);

    //Enabled or not -- load from saved value in case vinyl control is restarting
    m_bIsEnabled = wantenabled->get() > 0.0;

    // Load VC pre-amp gain from the config.
    // TODO(rryan): Should probably live in VinylControlManager since it's not
    // specific to a VC deck.
    ControlObject::set(ConfigKey(VINYL_PREF_KEY, "gain"),
        m_pConfig->getValueString(ConfigKey(VINYL_PREF_KEY,"gain")).toInt());
}
bool VinylControl::isEnabled()
{
    return m_bIsEnabled;
}
void VinylControl::toggleVinylControl(bool enable) {
    if (m_pConfig) m_pConfig->set(ConfigKey(m_group,"vinylcontrol_enabled"), ConfigValue((int)enable));
    enabled->set(enable);
}
VinylControl::~VinylControl()
{
    bool wasEnabled = m_bIsEnabled;
    enabled->set(false);
    vinylStatus->set(VINYL_STATUS_DISABLED);
    if (wasEnabled) {
        //if vinyl control is just restarting, indicate that it should
        //be enabled
        wantenabled->set(true);
    }
}
