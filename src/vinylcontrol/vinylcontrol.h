_Pragma("once")
#include <QString>

#include "util/types.h"
#include "configobject.h"
#include "vinylcontrol/vinylsignalquality.h"

class ControlObject;

class VinylControl : public QObject {
  public:
    VinylControl(ConfigObject<ConfigValue> *pConfig, QString group);
    virtual ~VinylControl();

    virtual void toggleVinylControl(bool enable);
    virtual bool isEnabled();
    virtual void analyzeSamples(CSAMPLE* pSamples, size_t nFrames) = 0;
    virtual bool writeQualityReport(VinylSignalQualityReport* qualityReportFifo) = 0;

  protected:
    virtual float getAngle() = 0;

    ConfigObject<ConfigValue>* m_pConfig;
    QString m_group;

    // The VC input gain preference.
    ControlObject* m_pVinylControlInputGain;

    ControlObject*playButton; //The ControlObject used to start/stop playback of the song.
    ControlObject*playPos; //The ControlObject used to read the playback position in the song.
    ControlObject*trackSamples;
    ControlObject*trackSampleRate;
    ControlObject*vinylSeek; //The ControlObject used to change the playback position in the song.
    // this rate is used in engine buffer for transport
    // 1.0 = original rate
    ControlObject* m_pVCRate;
    // Reflects the mean value (filtered for display) used of m_pVCRate during VC and
    // and is used to change the speed/pitch of the song without VC
    // 0.0 = original rate
    ControlObject* m_pRateSlider;
    ControlObject*duration; //The ControlObject used to get the duration of the current song.
    ControlObject*mode; //The ControlObject used to get the vinyl control mode (absolute/relative/scratch)
    ControlObject*enabled; //The ControlObject used to get if the vinyl control is enabled or disabled.
    ControlObject*wantenabled; //The ControlObject used to get if the vinyl control should try to enable itself
    ControlObject*cueing; //Should cueing mode be active?
    ControlObject*scratching; //Is pitch changing very quickly?
    ControlObject*rateRange; //The ControlObject used to the get the pitch range from the prefs.
    ControlObject*vinylStatus;
    ControlObject*rateDir; //direction of rate
    ControlObject*loopEnabled; //looping enabled?
    ControlObject*signalenabled; //show the signal in the skin?
    ControlObject*reverseButton; // When the user has pressed the "reverse" button.
    // The lead-in time...
    int m_iLeadInTime;
    // The position of the needle on the record as read by the VinylControl
    // implementation.
    double m_dVinylPosition;
    // Used as a measure of the quality of the timecode signal.
    float m_fTimecodeQuality;
    // Whether this VinylControl instance is enabled.
    bool m_bIsEnabled;
};
