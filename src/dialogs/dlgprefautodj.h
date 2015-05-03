#ifndef DLGPREFAUTODJ_H
#define DLGPREFAUTODJ_H

#include <QWidget>

#include "dialogs/ui_dlgprefautodjdlg.h"
#include "configobject.h"
#include "preferences/dlgpreferencepage.h"

class DlgPrefAutoDJ : public DlgPreferencePage, public Ui::DlgPrefAutoDJDlg {
    Q_OBJECT
  public:
    DlgPrefAutoDJ(QWidget* pParent, ConfigObject<ConfigValue> *pConfig);
    virtual ~DlgPrefAutoDJ();

  public slots:
    void onUpdate();
    void onApply();
    void onResetToDefaults();
    void onCancel() ;

  private slots:
    void onSetAutoDjRequeue(int);
    void onSetAutoDjMinimumAvailable(int);
    void onSetAutoDjUseIgnoreTime(int);
    void onSetAutoDjIgnoreTime(const QTime &a_rTime);
    void onSetAutoDJRandomQueueMin(int);
    void onEnableAutoDJRandomQueueComboBox(int);
    void onEnableAutoDJRandomQueue(int);

  private:
    ConfigObject<ConfigValue>* m_pConfig;
};

#endif /* DLGPREFAUTODJ_H */
