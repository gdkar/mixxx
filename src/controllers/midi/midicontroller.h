/**
* @file midicontroller.h
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Tue 7 Feb 2012
* @brief MIDI Controller base class
*
* This is a base class representing a MIDI controller.
*   It must be inherited by a class that implements it on some API.
*
*   Note that the subclass' destructor should call close() at a minimum.
*/

_Pragma("once")
#include <memory>
#include <vector>
#include "controllers/controller.h"
#include "controllers/midi/midicontrollerpreset.h"
#include "controllers/midi/midicontrollerpresetfilehandler.h"
#include "controllers/midi/midimessage.h"
#include "controllers/midi/midioutputhandler.h"
#include "controllers/softtakeover.h"
#include "util/duration.h"

class MidiController : public Controller {
    Q_OBJECT
  public:
    MidiController();
    virtual ~MidiController();
    virtual QString presetExtension();
    virtual ControllerPresetPointer getPreset() const;
    virtual bool savePreset(const QString fileName) const;
    virtual void visit(const ControllerPreset* preset);
    virtual bool isMappable() const;
    virtual bool matchPreset(const PresetInfo& preset);
  signals:
    void messageReceived(unsigned char status, unsigned char control,unsigned char value);
  protected:
    Q_INVOKABLE void sendShortMsg(unsigned char status, unsigned char byte1, unsigned char byte2);
    // Alias for send()
    Q_INVOKABLE void sendSysexMsg(QList<int> data, unsigned int length);
  protected slots:
    virtual void receive(unsigned char status, unsigned char control,
                         unsigned char value, mixxx::Duration timestamp);
    // For receiving System Exclusive messages
    virtual void receive(const QByteArray data, mixxx::Duration timestamp);
    virtual int close();
  private slots:
    // Initializes the engine and static output mappings.
    bool applyPreset(QList<QString> scriptPaths, bool initializeScripts);
    void learnTemporaryInputMappings(const MidiInputMappings& mappings);
    void clearTemporaryInputMappings();
    void commitTemporaryInputMappings();
  private:
    void processInputMapping(const MidiInputMapping& mapping,
                             unsigned char status,
                             unsigned char control,
                             unsigned char value,
                             mixxx::Duration timestamp);
    void processInputMapping(const MidiInputMapping& mapping,
                             const QByteArray& data,
                             mixxx::Duration timestamp);
    virtual void sendWord(unsigned int word) = 0;
    double computeValue(MidiOptions options, double _prevmidivalue, double _newmidivalue);
    void createOutputHandlers();
    void updateAllOutputs();
    void destroyOutputHandlers();
    // Returns a pointer to the currently loaded controller preset. For internal
    // use only.
    ControllerPreset* preset();
    QHash<uint16_t, MidiInputMapping> m_temporaryInputMappings;
    std::vector<std::unique_ptr<MidiOutputHandler> > m_outputs;
    MidiControllerPreset m_preset;
    SoftTakeoverCtrl m_st;
    QList<QPair<MidiInputMapping, unsigned char> > m_fourteen_bit_queued_mappings;
    // So it can access sendShortMsg()
    friend class MidiOutputHandler;
    friend class MidiControllerTest;
};
