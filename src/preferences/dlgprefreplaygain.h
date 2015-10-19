_Pragma("once")
#include <QWidget>

#include "preferences/ui_dlgprefreplaygaindlg.h"
#include "preferences/dlgpreferencepage.h"
#include "configobject.h"
class ControlObject;
class DlgPrefReplayGain: public DlgPreferencePage,
                         public Ui::DlgPrefReplayGainDlg {
    Q_OBJECT
  public:
    DlgPrefReplayGain(QWidget *parent, ConfigObject<ConfigValue> *_config);
    virtual ~DlgPrefReplayGain();

  public slots:
    // Update initial gain increment
    void slotUpdateReplayGainBoost();
    void slotUpdateDefaultBoost();
    void slotSetRGEnabled();
    void slotSetRGAnalyserEnabled();

    void slotApply();
    void slotUpdate();
    void slotResetToDefaults();

  signals:
    void apply(const QString &);

  private:
    // Determines whether or not to gray out the preferences
    void loadSettings();
    void setLabelCurrentReplayGainBoost(int value);

    // Pointer to config object
    ConfigObject<ConfigValue>* config;

    ControlObject* m_replayGainBoost;
    ControlObject* m_defaultBoost;
    ControlObject* m_enabled;
};
