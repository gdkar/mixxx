#ifndef CONTROLLERS_MIDI_PORTMIDIDEVICE
#define CONTROLLERS_MIDI_PORTMIDIDEVICE

#include <portmidi.h>
class PortMidiDevice {
  public:
    PortMidiDevice(const PmDeviceInfo* deviceInfo,
                   int deviceIndex)
            : m_pDeviceInfo(deviceInfo),
              m_deviceIndex(deviceIndex),
              m_pStream(nullptr) {
    }
    virtual ~PortMidiDevice() = default;
    virtual const PmDeviceInfo* info() const { return m_pDeviceInfo;}
    virtual int index() const { return m_deviceIndex; }
    virtual bool isOpen() const { return m_pStream; }
    virtual PmError openInput(int32_t bufferSize) {
        return Pm_OpenInput(&m_pStream, m_deviceIndex,
                            nullptr, // no drive hacks
                            bufferSize,
                            nullptr,
                            nullptr);
    }
    virtual PmError openOutput()
    {
        return Pm_OpenOutput(&m_pStream,
            m_deviceIndex,
            nullptr, // No driver hacks
            0, // No buffering
            nullptr, // Use PortTime for timing
            nullptr, // No time info
            0); // No latency compensation.
    }
    virtual PmError close()
    {
        auto err = Pm_Close(m_pStream);
        m_pStream = nullptr;
        return err;
    }
    virtual PmError poll()
    {
        return Pm_Poll(m_pStream);
    }
    virtual int read(PmEvent* buffer, int32_t length)
    {
        return Pm_Read(m_pStream, buffer, length);
    }
    virtual PmError writeShort(int32_t message)
    {
        return Pm_WriteShort(m_pStream, 0, message);
    }
    virtual PmError writeSysEx(unsigned char* message)
    {
        return Pm_WriteSysEx(m_pStream, 0, message);
    }
  private:
    const PmDeviceInfo* m_pDeviceInfo;
    int                 m_deviceIndex;
    PortMidiStream     *m_pStream;
};
#endif /* CONTROLLERS_MIDI_PORTMIDIDEVICE */
