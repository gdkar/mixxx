#include "engine/enginesidechaincompressor.h"
#include "controlpotmeter.h"
#include "controlpushbutton.h"

class ConfigValue;
class ControlObjectSlave;

class EngineTalkoverDucking :  public EngineSideChainCompressor {
  Q_OBJECT
  public:
    enum TalkoverDuckSetting {
        OFF = 0,
        AUTO,
        MANUAL,
    };
    EngineTalkoverDucking(const QString &group, ConfigObject<ConfigValue>* pConfig,  QObject *pParent=nullptr);
    virtual ~EngineTalkoverDucking();
    TalkoverDuckSetting getMode() const {
        return static_cast<TalkoverDuckSetting>(int(m_pTalkoverDucking->get()));
    }
    CSAMPLE getGain(int numFrames);
  public slots:
    const QString &group()const{return m_group;}
    void slotSampleRateChanged(double);
    void slotDuckStrengthChanged(double);
    void slotDuckModeChanged(double);
  private:
    const QString m_group;
    ConfigObject<ConfigValue>* m_pConfig;
    ControlObjectSlave* m_pMasterSampleRate;
    ControlPotmeter* m_pDuckStrength;
    ControlPushButton* m_pTalkoverDucking;
};
