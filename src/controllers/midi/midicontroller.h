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

#ifndef MIDICONTROLLER_H
#define MIDICONTROLLER_H

#include "controllers/controller.h"
#include "controllers/midi/midimessage.h"
#include "controllers/softtakeover.h"
#include "util/duration.h"

class MidiController : public Controller {
    Q_OBJECT
  public:
    MidiController(QObject *p = nullptr);
    virtual ~MidiController();

    QString presetExtension() const override;

  signals:
    void messageReceived(uint8_t status, uint8_t control,uint8_t value, double timestamp);
  protected:
    Q_INVOKABLE void sendShortMsg(unsigned int status, unsigned int byte1, unsigned int byte2);
    // Alias for send()
    Q_INVOKABLE void sendSysexMsg(QList<int> data, unsigned int length);
  protected slots:
    virtual void receive(unsigned char status, unsigned char control,
                         unsigned char value, mixxx::Duration timestamp);
    // For receiving System Exclusive messages
    void receive(QVariant data, mixxx::Duration timestamp) override;
    int close() override;
  private:

    virtual void sendWord(unsigned int word) = 0;
    double computeValue(MidiOptions options, double _prevmidivalue, double _newmidivalue);
    // Returns a pointer to the currently loaded controller preset. For internal
    // use only.
    SoftTakeoverCtrl m_st;
    QList<std::pair<MidiInputMapping, unsigned char> > m_fourteen_bit_queued_mappings;
    // So it can access sendShortMsg()
    friend class MidiOutputHandler;
    friend class MidiControllerTest;
};

#endif
