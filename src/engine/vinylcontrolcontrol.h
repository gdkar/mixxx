_Pragma("once")
#include "engine/enginecontrol.h"
#include "configobject.h"
class ControlPushButton;
class ControlObject;
class ControlObjectSlave;
class VinylControlControl : public EngineControl {
    Q_OBJECT
  public:
    VinylControlControl(QString group, ConfigObject<ConfigValue>* pConfig,QObject *pParent);
    virtual ~VinylControlControl();
    void trackLoaded(TrackPointer pTrack);
    void trackUnloaded(TrackPointer pTrack);
    // If the engine asks for a seek, we may need to disable absolute mode.
    void notifySeekQueued();
    bool isEnabled();
    bool isScratching();
  private slots:
    void slotControlVinylSeek(double fractionalPos);
  private:
    ControlObject* m_pControlVinylRate = nullptr;
    ControlObject* m_pControlVinylSeek = nullptr;
    ControlObject* m_pControlVinylSpeedType = nullptr;
    ControlObject* m_pControlVinylStatus = nullptr;
    ControlPushButton* m_pControlVinylScratching = nullptr;
    ControlPushButton* m_pControlVinylMode = nullptr;
    ControlPushButton* m_pControlVinylEnabled = nullptr;
    ControlPushButton* m_pControlVinylWantEnabled = nullptr;
    ControlPushButton* m_pControlVinylCueing = nullptr;
    ControlPushButton* m_pControlVinylSignalEnabled = nullptr;
    ControlObjectSlave* m_pPlayEnabled = nullptr;
    TrackPointer m_pCurrentTrack{nullptr};
    bool m_bSeekRequested{false};
};
