#include "engine/enginesidechaincompressor.h"
#include "preferences/usersettings.h"
class ControlProxy;
class ControlObject;

class EngineTalkoverDucking : public QObject, public EngineSideChainCompressor {
  Q_OBJECT
  public:

    enum TalkoverDuckSetting {
        OFF = 0,
        AUTO,
        MANUAL,
    };
    Q_ENUM(TalkoverDuckSetting);
    EngineTalkoverDucking(UserSettingsPointer pConfig, const char* group);
    virtual ~EngineTalkoverDucking();
    TalkoverDuckSetting getMode() const;
    CSAMPLE getGain(int numFrames);
  public slots:
    void slotSampleRateChanged(double);
    void slotDuckStrengthChanged(double);
    void slotDuckModeChanged(double);

  private:
    UserSettingsPointer m_pConfig;
    const char* m_group;

    ControlProxy* m_pMasterSampleRate;
    ControlObject* m_pDuckStrength;
    ControlObject* m_pTalkoverDucking;
};
