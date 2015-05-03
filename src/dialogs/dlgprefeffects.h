#ifndef DLGPREFEFFECTS_H
#define DLGPREFEFFECTS_H

#include "configobject.h"
#include "dialogs/ui_dlgprefeffectsdlg.h"
#include "preferences/dlgpreferencepage.h"

class EffectsManager;

class DlgPrefEffects : public DlgPreferencePage, public Ui::DlgPrefEffectsDlg {
    Q_OBJECT
  public:
    DlgPrefEffects(QWidget* pParent,
                   ConfigObject<ConfigValue>* pConfig,
                   EffectsManager* pEffectsManager);
    virtual ~DlgPrefEffects() {}

    void onUpdate();
    void onApply();
    void onResetToDefaults();

  private slots:
    void onEffectSelected(QListWidgetItem* pCurrent,
                            QListWidgetItem* pPrevious);

  private:
    void addEffectToList(const QString& id);
    void clear();

    ConfigObject<ConfigValue>* m_pConfig;
    EffectsManager* m_pEffectsManager;
};

#endif /* DLGPREFEFFECTS_H */
