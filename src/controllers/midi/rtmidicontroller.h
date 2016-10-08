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

struct RtMidiControllerInfo {
    Q_PROPERTY(QString inputName MEMBER inputName)
    Q_PROPERTY(QString outputName MEMBER outputName)
    Q_PROPERTY(int inputIndex MEMBER inputIndex)
    Q_PROPERTY(int outputIndex MEMBER outputIndex)
    Q_GADGET
public:
    RtMidiControllerInfo() = default;
    RtMidiControllerInfo(const RtMidiControllerInfo&) = default;
    RtMidiControllerInfo(RtMidiControllerInfo&&) noexcept = default;
    RtMidiControllerInfo&operator=(const RtMidiControllerInfo&) = default;
    RtMidiControllerInfo&operator=(RtMidiControllerInfo&&) noexcept = default;
    RtMidiControllerInfo(const QString &iname, const QString &oname, int iidx = -1, int iodx = -1);
    virtual ~RtMidiControllerInfo();
    QString inputName{};
    QString outputName{};
    int     inputIndex{};
    int     outputIndex{};
};
Q_DECLARE_METATYPE(RtMidiControllerInfo)

// A PortMidi-based implementation of MidiController
class RtMidiController : public MidiController {
    Q_OBJECT
    Q_PROPERTY(int inputPortCount READ inputPortCount NOTIFY inputPortCountChanged)
    Q_PROPERTY(int outputPortCount READ outputPortCount NOTIFY outputPortCountChanged)
    Q_PROPERTY(QStringList inputPortNames READ inputPortNames)
    Q_PROPERTY(QStringList outputPortNames READ outputPortNames)
    Q_PROPERTY(QList<RtMidiControllerInfo> devices READ deviceList)

    Q_PROPERTY(int inputIndex READ inputIndex WRITE setInputIndex NOTIFY inputIndexChanged)
    Q_PROPERTY(int outputIndex READ outputIndex WRITE setOutputIndex NOTIFY outputIndexChanged)

    Q_PROPERTY(QString inputName READ inputName NOTIFY inputNameChanged)
    Q_PROPERTY(QString outputName READ outputName NOTIFY outputNameChanged)
  public:
    Q_INVOKABLE RtMidiController(QObject *p = nullptr);
    RtMidiController(int inputDeviceIndex, QString inputDeviceName,
                     int outputDeviceIndex,QString outputDeviceName, QObject *p = nullptr);
   ~RtMidiController();

  public slots:
    int open() override;
    int close() override;
    bool poll() override;
    QString inputName() const;
    QString outputName() const;
    int inputPortCount() const;
    int outputPortCount() const;
    int inputIndex() const;
    int outputIndex() const;
    void setInputIndex(int);
    void setOutputIndex(int);
    Q_INVOKABLE QString inputPortName(int index) const;
    Q_INVOKABLE QString outputPortName(int index) const;
  signals:
    void inputPortCountChanged(int);
    void outputPortCountChanged(int);

    void inputIndexChanged(int);
    void outputIndexChanged(int);
    void inputNameChanged(QString);
    void outputNameChanged(QString);
  protected:
    static void trampoline(
        double deltatime
      , std::vector<uint8_t> * message
      , void *opaque);

    void callback(double deltatime, std::vector<uint8_t>&message);
    // The sysex data must already contain the start byte 0xf0 and the end byte
    // 0xf7.
    void sendWord(uint32_t word) override;
    void send(QByteArray data) override;
    bool isPolling() const override;
    QStringList inputPortNames() const;
    QStringList outputPortNames() const;
    QList<RtMidiControllerInfo> deviceList() const;
    int in_index{-1};
    QString in_name{};
    int out_index{-1};
    QString out_name{};
    std::unique_ptr<RtMidiIn>  m_midiIn {};
    std::unique_ptr<RtMidiOut> m_midiOut{};
    // Storage for SysEx messages
    std::vector<uint8_t> m_sysex{};
    bool m_bInSysex{false};
};
#endif
