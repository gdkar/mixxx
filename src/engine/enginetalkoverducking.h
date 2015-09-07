#include "engine/enginesidechaincompressor.h"
#include "configobject.h"
class ConfigValue;
class ControlObjectSlave;
class ControlPushButton;
class ControlPotmeter;
class EngineTalkoverDucking : public QObject, public EngineSideChainCompressor {
  Q_OBJECT
  public:
    enum TalkoverDuckSetting {
        OFF = 0,
        AUTO,
        MANUAL,
    };
    Q_ENUMS(TalkoverDuckSetting);
    EngineTalkoverDucking(ConfigObject<ConfigValue>* pConfig, const char* group);
    virtual ~EngineTalkoverDucking();
    TalkoverDuckSetting getMode() const ;
    CSAMPLE getGain(int numFrames);
  public slots:
    void slotSampleRateChanged(double);
    void slotDuckStrengthChanged(double);
    void slotDuckModeChanged(double);
  private:
    ConfigObject<ConfigValue>* m_pConfig = nullptr;
    const char* m_group;
    ControlObjectSlave* m_pMasterSampleRate = nullptr;
    ControlPotmeter* m_pDuckStrength = nullptr;
    ControlPushButton* m_pTalkoverDucking = nullptr;
};
