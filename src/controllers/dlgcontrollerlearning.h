/**
* @file dlgcontrollerlearning.h
* @author Sean M. Pappalardo  spappalardo@mixxx.org
* @date Thu 12 Apr 2012
* @brief The controller mapping learning wizard
*
*/
_Pragma("once")
#include <QDialog>
#include <QList>
#include <QString>
#include <QTimer>

#include "controllers/ui_dlgcontrollerlearning.h"
#include "controllers/controlpickermenu.h"
#include "controllers/midi/midicontroller.h"
#ifdef __HID__
#include "controllers/hid/hidcontroller.h"
#endif
#include "controllers/bulk/bulkcontroller.h"
#include "controllers/midi/midimessage.h"
#include "controllers/controller.h"
#include "controllers/controllervisitor.h"
#include "configobject.h"
class ControllerPreset;
//#define CONTROLLERLESSTESTING
class DlgControllerLearning : public QDialog, public ControllerVisitor, public Ui::DlgControllerLearning {
    Q_OBJECT
  public:
    DlgControllerLearning(QWidget *parent, Controller *controller);
    virtual ~DlgControllerLearning();
    virtual void visit(Controller* pController);
  signals:
    void learnTemporaryInputMappings(const MidiInputMappings& mappings);
    void clearTemporaryInputMappings();
    void commitTemporaryInputMappings();
    // Used to notify DlgPrefController that we have learned some new input
    // mappings.
    void inputMappingsLearned(const MidiInputMappings& mappings);
    void startLearning();
    void stopLearning();
    void listenForClicks();
    void stopListeningForClicks();
  public slots:
    // Triggered when the user picks a control from the menu.
    virtual void controlPicked(ConfigKey control);
    // Triggered when user clicks a control from the GUI
    virtual void controlClicked(ControlObject* pControl);
    virtual void comboboxIndexChanged(int index);
    virtual void slotMessageReceived(unsigned char status, unsigned char control, unsigned char value);
    virtual void slotCancelLearn();
    virtual void slotChooseControlPressed();
    virtual void slotTimerExpired();
    virtual void slotFirstMessageTimeout();
    virtual void slotRetry();
    virtual void slotStartLearningPressed();
    virtual void slotMidiOptionsChanged();
  private slots:
    virtual void showControlMenu();
#ifdef CONTROLLERLESSTESTING
    void DEBUGFakeMidiMessage();
    void DEBUGFakeMidiMessage2();
#endif
  private:
    virtual void loadControl(const ConfigKey& key, QString title, QString description);
    virtual void startListening();
    virtual void commitMapping();
    virtual void resetWizard(bool keepCurrentControl = false);
    void populateComboBox();
    Controller* m_pController;
    MidiController* m_pMidiController;
    ControlPickerMenu m_controlPickerMenu;
    ConfigKey m_currentControl;
    bool m_messagesLearned;
    QTimer m_firstMessageTimer;
    QTimer m_lastMessageTimer;
    QList<QPair<MidiKey, unsigned char> > m_messages;
    MidiInputMappings m_mappings;
};
