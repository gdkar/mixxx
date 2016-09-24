/**
* @file dlgprefcontroller.h
* @author Sean M. Pappalardo  spappalardo@mixxx.org
* @date Mon May 2 2011
* @brief Configuration dialog for a DJ controller
*/

#ifndef DLGPREFCONTROLLER_H
#define DLGPREFCONTROLLER_H

#include <QHash>
#include <QSortFilterProxyModel>

//#include "controllers/dlgcontrollerlearning.h"
#include "controllers/ui_dlgprefcontrollerdlg.h"
#include "preferences/usersettings.h"
#include "preferences/dlgpreferencepage.h"

// Forward declarations
class Controller;
class ControllerManager;

class DlgPrefController : public DlgPreferencePage {
    Q_OBJECT
  public:
    DlgPrefController(QWidget *parent, Controller* controller,
                      ControllerManager* controllerManager,
                      UserSettingsPointer pConfig);
    virtual ~DlgPrefController();

  public slots:
    // Called when we should apply / save our changes.
    void onApply();
    // Called when we should cancel the changes made.
    void onCancel();
    // Called when preference dialog (not this dialog) is displayed.
    void onUpdate();
    // Called when the user toggles the enabled checkbox.
    void onEnableDevice(bool enable);
    // Called when the user selects a preset from the combobox.
    // Mark that we need to apply the settings.
    void onDirty();

  signals:
    void controllerEnabled(DlgPrefController*, bool);
    void openController(Controller* pController);
    void closeController(Controller* pController);
//    void mappingStarted();
//    void mappingEnded();

  private slots:
    // Input mappings
//    void addInputMapping();
    void showLearningWizard();
//    void removeInputMappings();
//    void clearAllInputMappings();

    // Output mappings
//    void addOutputMapping();
//    void removeOutputMappings();
//    void clearAllOutputMappings();

    // Scripts
    void addScript();
    void removeScript();
    void openScript();

//    void midiInputMappingsLearned(const MidiInputMappings& mappings);

  private:
    void initTableView(QTableView* pTable);

    // Reload the mappings in the dropdown dialog
    void enableDevice();
    void disableDevice();

    Ui::DlgPrefControllerDlg m_ui;
    UserSettingsPointer m_pConfig;
    ControllerManager* m_pControllerManager;
    Controller* m_pController;
//    DlgControllerLearning* m_pDlgControllerLearning;
//    ControllerInputMappingTableModel* m_pInputTableModel;
    QSortFilterProxyModel* m_pInputProxyModel;
//    ControllerOutputMappingTableModel* m_pOutputTableModel;
    QSortFilterProxyModel* m_pOutputProxyModel;
    bool m_bDirty;
};
#endif /*DLGPREFCONTROLLER_H*/
