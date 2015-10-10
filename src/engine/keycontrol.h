_Pragma("once")
#include "engine/enginecontrol.h"
#include <atomic>

class ControlObject;
class ControlPotmeter;
class ControlPushButton;

class KeyControl : public EngineControl {
    Q_OBJECT
  public:
    struct PitchTempoRatio {
        // this is the calculated value used by engine buffer for pitch
        // by default is is equal to the tempoRatio set by the speed slider
        double pitchRatio      = 1.0;
        // this is the value of the speed slider and speed slider
        // effecting controls at the moment of calculation
        double tempoRatio      = 1.0;
        // the offeset factor to the natural pitch set by the rate slider
        double pitchTweakRatio = 1.0;
        bool keylock           = false;
    };
    KeyControl(QString group, ConfigObject<ConfigValue>* pConfig,QObject *);
    virtual ~KeyControl();
    // Returns a struct, with the results of the last pitch and tempo calculations
    KeyControl::PitchTempoRatio getPitchTempoRatio();
    double getKey();
    void collectFeatures(GroupFeatureState* pGroupFeatures) const;
  private slots:
    void slotSetEngineKey(double);
    void slotSetEngineKeyDistance(double);
    void slotFileKeyChanged(double);
    void slotPitchChanged(double);
    void slotPitchAdjustChanged(double);
    void slotRateChanged();
    void slotSyncKey(double);
    void slotResetKey(double);
  private:
    void setEngineKey(double key, double key_distance);
    bool syncKey(EngineBuffer* pOtherEngineBuffer);
    void updateKeyCOs(double fileKeyNumeric, double pitch);
    void updatePitch();
    void updatePitchAdjust();
    void updateRate();

    // ControlObjects that come from EngineBuffer
    ControlObject* m_pRateSlider= nullptr;
    ControlObject* m_pRateRange = nullptr;
    ControlObject* m_pRateDir   = nullptr;

    ControlObject* m_pVCRate    = nullptr;
    ControlObject* m_pVCEnabled = nullptr;

    ControlObject* m_pKeylock = nullptr;
    ControlPotmeter* m_pPitch = nullptr;
    ControlPotmeter* m_pPitchAdjust = nullptr;
    ControlPushButton* m_pButtonSyncKey = nullptr;
    ControlPushButton* m_pButtonResetKey = nullptr;
    ControlPushButton* m_keylockMode = nullptr;
    /** The current loaded file's detected key */
    ControlObject* m_pFileKey             = nullptr;
    /** The current effective key of the engine */
    ControlObject* m_pEngineKey           = nullptr;
    ControlPotmeter* m_pEngineKeyDistance = nullptr;
    TrackPointer m_pTrack{nullptr};
    PitchTempoRatio m_pitchRateInfo;
    std::atomic<bool> m_updatePitchRequest;
    std::atomic<bool> m_updatePitchAdjustRequest;
    std::atomic<bool> m_updateRateRequest;
};
