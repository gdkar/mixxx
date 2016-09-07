/**
 * @file portmidicontroller.h
 * @author Albert Santoni alberts@mixxx.org
 * @author Sean M. Pappalardo  spappalardo@mixxx.org
 * @date Thu 15 Mar 2012
 * @brief PortMidi-based MIDI backend
 *
 * This class is represents a MIDI device, either physical or software.
 * It uses the PortMidi API to send and receive MIDI messages to/from the device.
 * It's important to note that PortMidi treats input and output on a single
 * physical device as two separate half-duplex devices. In this class, we wrap
 * those together into a single device, which is why the constructor takes
 * both arguments pertaining to both input and output "devices".
 *
 */

#ifndef RTMIDICONTROLLER_H
#define RTMIDICONTROLLER_H

#include <rtmidi/RtMidi.h>

#include <memory>
#include "controllers/midi/midicontroller.h"


// A PortMidi-based implementation of MidiController
class RtMidiController : public MidiController {
    Q_OBJECT
  public:
    RtMidiController(int inputDeviceIndex, const std::string &inputDeviceName,
                     int outputDeviceIndex,const std::string &outputDeviceName);
    virtual ~RtMidiController();
  public slots:
    virtual int open();
    virtual int close();
    virtual bool poll(){return false;}
  protected:
    static void trampoline(double deltatime, std::vector<unsigned char> * message, void *opaque)
    {
        if(message && opaque)
            static_cast<RtMidiController*>(opaque)->callback(deltatime, *message);
    }
    void callback(double deltatime, std::vector<unsigned char>&message);
    // The sysex data must already contain the start byte 0xf0 and the end byte
    // 0xf7.
    void sendWord(unsigned int word) override;
    void send(QByteArray data);
    bool isPolling() const override { return false; }
    int in_index{-1};
    std::string in_name{};
    int out_index{-1};
    std::string out_name{};
    std::unique_ptr<RtMidiIn>  m_midiIn{};
    std::unique_ptr<RtMidiOut> m_midiOut{};

    // Storage for SysEx messages
    std::vector<unsigned char> m_sysex{};
    bool          m_bInSysex{false};
};
#endif
