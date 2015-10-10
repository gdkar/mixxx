// ratecontrol.h
// Created 7/4/2009 by RJ Ryan (rryan@mit.edu)

_Pragma("once")
#include <QObject>

#include "configobject.h"
#include "engine/enginecontrol.h"
#include "engine/sync/syncable.h"

const int RATE_TEMP_STEP = 500;
const int RATE_TEMP_STEP_SMALL = RATE_TEMP_STEP * 10.;
const int RATE_SENSITIVITY_MIN = 100;
const int RATE_SENSITIVITY_MAX = 2500;

class BpmControl;
class ControlTTRotary;
class ControlObject;
class ControlPotmeter;
class ControlPushButton;
class ControlObjectSlave;
class EngineChannel;
class PositionScratchController;

// RateControl is an EngineControl that is in charge of managing the rate of
// playback of a given channel of audio in the Mixxx engine. Using input from
// various controls, RateControl will calculate the current rate.
class RateControl : public EngineControl {
    Q_OBJECT
public:
    RateControl(QString group, ConfigObject<ConfigValue>* _config, QObject *);
    virtual ~RateControl();
    // Enumerations which hold the state of the pitchbend buttons.
    // These enumerations can be used like a bitmask.
    // The current rate ramping direction. Only holds the last button pressed.

    enum RATERAMP_DIRECTION
    {
        RATERAMP_NONE = 0,  // No buttons are held down
        RATERAMP_DOWN = 1,  // Down button is being held
        RATERAMP_UP = 2,    // Up button is being held
        RATERAMP_BOTH = 3   // Both buttons are being held down
    };
    Q_ENUM(RATERAMP_DIRECTION);
    // Rate ramping mode:
    //  RATERAMP_STEP: pitch takes a temporary step up/down a certain amount.
    //  RATERAMP_LINEAR: pitch moves up/down in a progresively linear fashion.
    enum RATERAMP_MODE
    {
        RATERAMP_STEP = 0,
        RATERAMP_LINEAR = 1
    };
    Q_ENUM(RATERAMP_MODE);
    // This defines how the rate returns to normal. Currently unused.
    // Rate ramp back mode:
    //  RATERAMP_RAMPBACK_NONE: returns back to normal all at once.
    //  RATERAMP_RAMPBACK_SPEED: moves back in a linearly progresive manner.
    //  RATERAMP_RAMPBACK_PERIOD: returns to normal within a period of time.
    enum RATERAMP_RAMPBACK_MODE
    {
        RATERAMP_RAMPBACK_NONE,
        RATERAMP_RAMPBACK_SPEED,
        RATERAMP_RAMPBACK_PERIOD
    };
    Q_ENUM(RATERAMP_RAMPBACK_MODE);
    void setBpmControl(BpmControl* bpmcontrol);
    // Must be called during each callback of the audio thread so that
    // RateControl has a chance to update itself.
    double process(double dRate,double currentSample,double totalSamples,int bufferSamples);
    // Returns the current engine rate.  "reportScratching" is used to tell
    // the caller that the user is currently scratching, and this is used to
    // disable keylock.
    double calculateSpeed(double baserate, double speed, bool paused,int iSamplesPerBuffer, bool* pReportScratching,bool* pReportReverse);
    double getRawRate() const;
    // Set rate change when temp rate button is pressed
    static void setTemp(double v);
    // Set rate change when temp rate small button is pressed
    static void setTempSmall(double v);
    // Set rate change when perm rate button is pressed
    static void setPerm(double v);
    // Set rate change when perm rate small button is pressed
    static void setPermSmall(double v);
    // Set Rate Ramp Mode
    static void setRateRamp(bool);
    // Set Rate Ramp Sensitivity
    static void setRateRampSensitivity(int);
    virtual void notifySeek(double dNewPlaypos);

  public slots:
    void slotReverseRollActivate(double);
    void slotControlRatePermDown(double);
    void slotControlRatePermDownSmall(double);
    void slotControlRatePermUp(double);
    void slotControlRatePermUpSmall(double);
    void slotControlRateTempDown(double);
    void slotControlRateTempDownSmall(double);
    void slotControlRateTempUp(double);
    void slotControlRateTempUpSmall(double);
    void slotControlFastForward(double);
    void slotControlFastBack(double);
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);

  private:
    double getJogFactor() ;
    double getWheelFactor() const;
    SyncMode getSyncMode() const;
    /** Set rate change of the temporary pitch rate */
    void setRateTemp(double v);
    /** Add a value to the temporary pitch rate */
    void addRateTemp(double v);
    /** Subtract a value from the temporary pitch rate */
    void subRateTemp(double v);
    /** Reset the temporary pitch rate */
    void resetRateTemp(void);
    /** Get the 'Raw' Temp Rate */
    double getTempRate(void);
    /** Values used when temp and perm rate buttons are pressed */
    static double m_dTemp, m_dTempSmall, m_dPerm, m_dPermSmall;

    ControlPushButton *buttonRateTempDown = nullptr,
                      *buttonRateTempDownSmall = nullptr,
                      *buttonRateTempUp = nullptr,
                      *buttonRateTempUpSmall = nullptr;
    ControlPushButton *buttonRatePermDown = nullptr,
                      *buttonRatePermDownSmall = nullptr,
                      *buttonRatePermUp = nullptr,
                      *buttonRatePermUpSmall = nullptr;
    ControlObject *m_pRateDir = nullptr,
                  *m_pRateRange = nullptr;
    ControlPotmeter* m_pRateSlider = nullptr;
    ControlPotmeter* m_pRateSearch = nullptr;
    ControlPushButton* m_pReverseButton = nullptr;
    ControlPushButton* m_pReverseRollButton = nullptr;
    ControlObject* m_pBackButton = nullptr;
    ControlObject* m_pForwardButton = nullptr;
    ControlTTRotary* m_pWheel = nullptr;
    ControlObject* m_pScratch2 = nullptr;
    PositionScratchController* m_pScratchController = nullptr;
    ControlPushButton* m_pScratch2Enable = nullptr;
    ControlObject* m_pJog = nullptr;
    double         m_jogFilter[32];
    double         m_jogAccum = 0;
    int            m_jogIndex = 0;
    ControlObject* m_pVCEnabled = nullptr;
    ControlObject* m_pVCScratching = nullptr;
    ControlObject* m_pVCMode = nullptr;
    ControlObject* m_pScratch2Scratching = nullptr;
    ControlObject* m_pSampleRate = nullptr;
    TrackPointer m_pTrack;
    // For Master Sync
    BpmControl* m_pBpmControl = nullptr;
    ControlObjectSlave* m_pSyncMode = nullptr;
    ControlObjectSlave* m_pSlipEnabled = nullptr;
    int m_ePbCurrent = 0;
    //  The rate ramping buttons which are currently being pressed.
    int m_ePbPressed = 0;
    // This is true if we've already started to ramp the rate
    bool m_bTempStarted = false;
    // Set to the rate change used for rate temp
    double m_dTempRateChange = 0.0;
    // Set the Temporary Rate Change Mode
    static enum RATERAMP_MODE m_eRateRampMode;
    // The Rate Temp Sensitivity, the higher it is the slower it gets
    static int m_iRateRampSensitivity;
    // Factor applied to the deprecated "wheel" rate value.
    static const double kWheelMultiplier;
    // Factor applied to jogwheels when the track is paused to speed up seeking.
    static const double kPausedJogMultiplier;
    // Temporary pitchrate, added to the permanent rate for calculateRate
    double m_dRateTemp = 0.0;
    enum RATERAMP_RAMPBACK_MODE m_eRampBackMode;
    // Return speed for temporary rate change
    double m_dRateTempRampbackChange = 0;
    bool m_reverseChanged = false;
};
