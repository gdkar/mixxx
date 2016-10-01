/**
 * @file portmidicontroller.h
 * @author Albert Santoni alberts@mixxx.org
 * @author Sean M. Pappalardo  spappalardo@mixxx.org
 * @date Thu 15 Mar 2012
 * @brief PortMidi-based MIDI backend
 *
 */

#include "controllers/midi/portmidicontroller.h"
#include "controllers/controllerdebug.h"

PortMidiController::PortMidiController(QObject *p )
: MidiController(p)
{}
PortMidiController::PortMidiController(const PmDeviceInfo* inputDeviceInfo,
                                       const PmDeviceInfo* outputDeviceInfo,
                                       int inputDeviceIndex,
                                       int outputDeviceIndex)
        : MidiController(),
          m_bInSysex(false) {
    for (unsigned int k = 0; k < MIXXX_PORTMIDI_BUFFER_LEN; ++k) {
        // Can be shortened to `m_midiBuffer[k] = {}` with C++11.
        m_midiBuffer[k].message = 0;
        m_midiBuffer[k].timestamp = 0;
    }
    // Note: We prepend the input stream's index to the device's name to prevent
    // duplicate devices from causing mayhem.
    //setDeviceName(QString("%1. %2").arg(QString::number(m_iInputDeviceIndex), inputDeviceInfo->name));
    if (inputDeviceInfo) {
        setDeviceName(QString("PortMidi: %1").arg(inputDeviceInfo->name));
        setInputDevice(inputDeviceInfo->input);
        m_pInputDevice = std::make_unique<PortMidiDevice>(inputDeviceInfo, inputDeviceIndex);
    }
    if (outputDeviceInfo) {
        if (inputDeviceInfo == NULL) {
            setDeviceName(QString("PortMidi: %1").arg(outputDeviceInfo->name));
        }
        setOutputDevice(outputDeviceInfo->output);
        m_pOutputDevice = std::make_unique<PortMidiDevice>(outputDeviceInfo, outputDeviceIndex);
    }
}

PortMidiController::~PortMidiController()
{
    if (isOpen()) {
        close();
    }
}

int PortMidiController::open() {
    if (isOpen()) {
        qDebug() << "PortMIDI device" << getDeviceName() << "already open";
        return -1;
    }

    if (getDeviceName() == MIXXX_PORTMIDI_NO_DEVICE_STRING)
        return -1;
    m_sysex.clear();
    m_bInSysex = false;

    if (m_pInputDevice && isInputDevice()) {
        controllerDebug("PortMidiController: Opening"
                        << m_pInputDevice->info()->name << "index"
                        << m_pInputDevice->index() << "for input");
        PmError err = m_pInputDevice->openInput(MIXXX_PORTMIDI_BUFFER_LEN);

        if (err != pmNoError) {
            qWarning() << "PortMidi error:" << Pm_GetErrorText(err);
            return -2;
        }
    }
    if (m_pOutputDevice && isOutputDevice()) {
        controllerDebug("PortMidiController: Opening"
                        << m_pOutputDevice->info()->name << "index"
                        << m_pOutputDevice->index() << "for output");

        PmError err = m_pOutputDevice->openOutput();
        if (err != pmNoError) {
            qWarning() << "PortMidi error:" << Pm_GetErrorText(err);
            return -2;
        }
    }

    setOpen(true);
//    startEngine();
    return 0;
}

int PortMidiController::close() {
    if (!isOpen()) {
        qDebug() << "PortMIDI device" << getDeviceName() << "already closed";
        return -1;
    }

//    stopEngine();
    MidiController::close();

    int result = 0;

    if (m_pInputDevice && m_pInputDevice->isOpen()) {
        PmError err = m_pInputDevice->close();
        if (err != pmNoError) {
            qWarning() << "PortMidi error:" << Pm_GetErrorText(err);
            result = -1;
        }
    }

    if (m_pOutputDevice && m_pOutputDevice->isOpen()) {
        PmError err = m_pOutputDevice->close();
        if (err != pmNoError) {
            qWarning() << "PortMidi error:" << Pm_GetErrorText(err);
            result = -1;
        }
    }

    setOpen(false);
    return result;
}
bool PortMidiController::poll()
{
    // Poll the controller for new data if it's an input device
    if (!m_pInputDevice || !m_pInputDevice->isOpen()) {
        return false;
    }
    // Returns true if events are available or an error code.
    PmError gotEvents = m_pInputDevice->poll();
    if (gotEvents == FALSE) {
        return false;
    }
    if (gotEvents < 0) {
        qWarning() << "PortMidi error:" << Pm_GetErrorText(gotEvents);
        return false;
    }
    auto numEvents = m_pInputDevice->read(m_midiBuffer, MIXXX_PORTMIDI_BUFFER_LEN);
    //qDebug() << "PortMidiController::poll()" << numEvents;

    if (numEvents < 0) {
        qWarning() << "PortMidi error:" << Pm_GetErrorText((PmError)numEvents);
        return false;
    }
    for (int i = 0; i < numEvents; i++) {
        auto status = Pm_MessageStatus(m_midiBuffer[i].message);
        auto timestamp = mixxx::Duration::fromMillis(m_midiBuffer[i].timestamp);

        if ((status & 0xF8) == 0xF8) {
            // Handle real-time MIDI messages at any time
            receive(status, 0, 0, timestamp);
            continue;
        }
        reprocessMessage:
        if (!m_bInSysex) {
            if (status == 0xF0) {
                m_bInSysex = true;
                status = 0;
            } else {
                //unsigned char channel = status & 0x0F;
                auto note = Pm_MessageData1(m_midiBuffer[i].message);
                auto velocity = Pm_MessageData2(m_midiBuffer[i].message);
                receive(status, note, velocity, timestamp);
            }
        }
        if (m_bInSysex) {
            // Abort (drop) the current System Exclusive message if a
            //  non-realtime status byte was received
            if (status > 0x7F && status < 0xF7) {
                m_bInSysex = false;
                m_sysex.clear();
                qWarning() << "Buggy MIDI device: SysEx interrupted!";
                goto reprocessMessage;    // Don't lose the new message
            }
            // Collect bytes from PmMessage
            uint8_t data = 0;
            for (int shift = 0; shift < 32 && (data != MIDI_EOX); shift += 8) {
                // TODO(rryan): This prevents buffer overflow if the sysex is
                // larger than 1024 bytes. I don't want to radically change
                // anything before the 2.0 release so this will do for now.
                data = (m_midiBuffer[i].message >> shift) & 0xFF;
                m_sysex.push_back(uint8_t(data));
            }
            // End System Exclusive message if the EOX byte was received
            if (data == MIDI_EOX) {
                m_bInSysex = false;
                const char* buffer = reinterpret_cast<const char*>(m_sysex.data());
//                receive(QByteArray::fromRawData(buffer, m_sysex.size()),timestamp);
                m_sysex.clear();
            }
        }
    }
    return numEvents > 0;
}
void PortMidiController::sendWord(unsigned int word)
{
    if (!m_pOutputDevice || !m_pOutputDevice->isOpen()) {
        return;
    }

    PmError err = m_pOutputDevice->writeShort(word);
    if (err != pmNoError) {
        qWarning() << "PortMidi sendShortMsg error:" << Pm_GetErrorText(err);
    }
}

void PortMidiController::send(QByteArray data)
{
    // PortMidi does not receive a length argument for the buffer we provide to
    // Pm_WriteSysEx. Instead, it scans for a MIDI_EOX byte to know when the
    // message is over. If one is not provided, it will overflow the buffer and
    // cause a segfault.
    if (!data.endsWith(MIDI_EOX)) {
        controllerDebug("SysEx message does not end with 0xF7 -- ignoring.");
        return;
    }
    if (!m_pOutputDevice || !m_pOutputDevice->isOpen()) {
        return;
    }
    PmError err = m_pOutputDevice->writeSysEx((unsigned char*)data.constData());
    if (err != pmNoError) {
        qWarning() << "PortMidi sendSysexMsg error:" << Pm_GetErrorText(err);
    }
}
