/**
 * @file portmidicontroller.h
 * @author Albert Santoni alberts@mixxx.org
 * @author Sean M. Pappalardo  spappalardo@mixxx.org
 * @date Thu 15 Mar 2012
 * @brief RtMidi-based MIDI backend
 *
 */

#include "controllers/midi/rtmidicontroller.h"
#include "controllers/controllerdebug.h"


RtMidiController::RtMidiController(QObject *p)
: MidiController(p)
, m_midiIn(std::make_unique<RtMidiIn>())
, m_midiOut(std::make_unique<RtMidiOut>())

{ }

RtMidiController::RtMidiController(int inIndex, QString inName, int outIndex, QString outName, QObject *p)
        : MidiController(p),
          in_index(inIndex),
          in_name(inName),
          out_index(outIndex),
          out_name(outName),
          m_midiIn(std::make_unique<RtMidiIn>()),
          m_midiOut(std::make_unique<RtMidiOut>())
{
    setInputDevice(in_index >= 0);
    setOutputDevice(out_index >= 0);

    if(!outName.isEmpty()) {
        setDeviceName("RtMidi: " + outName);
    }else if(!inName.isEmpty()) {
        setDeviceName("RtMidi: " + inName);
    }
}
int RtMidiController::inputPortCount() const
{
    try {
        return m_midiIn->getPortCount();
    }catch(...) {
        return 0;
    }
}
int RtMidiController::outputPortCount() const
{
    try {
        return m_midiOut->getPortCount();
    }catch(...) {
        return 0;
    }
}
QString RtMidiController::inputPortName(int index) const
{
    try {
        return QString::fromStdString(m_midiIn->getPortName(index));
    }catch(...) {
        return QString{};
    }
}
QString RtMidiController::outputPortName(int index) const
{
    try {
        return QString::fromStdString(m_midiOut->getPortName(index));
    }catch(...) {
        return QString{};
    }
}

int RtMidiController::inputIndex() const { return in_index; }
int RtMidiController::outputIndex() const { return out_index; }

QString RtMidiController::inputName() const { return in_name; }
QString RtMidiController::outputName() const { return out_name; }
void RtMidiController::setInputIndex(int _index)
{
    if(_index != in_index) {
        auto _open = isOpen();

        if(_open)
            close();

        in_index = _index;
        in_name.clear();

        if(_open)
            open();

        emit(inputIndexChanged(_index));
    }
}
void RtMidiController::setOutputIndex(int _index)
{
    if(_index != out_index) {
        auto _open = isOpen();
        if(isOpen()){
            close();
        }
        out_index = _index;
        out_name.clear();
        emit(outputIndexChanged(_index));
        if(_open)
            open();
    }
}
RtMidiController::~RtMidiController()
{
    if (isOpen())
        close();
}
int RtMidiController::open()
{
    if (isOpen()) {
        qDebug() << "RtMidiDevice" << getDeviceName() << "already open";
        return -1;
    }else{
        qDebug() << "opening RtMidiDevice" << getDeviceName();
    }
    if(in_index>= 0) {
        auto _iindex = in_index;
        auto _iname  = in_name;
        try {
            m_midiIn = std::make_unique<RtMidiIn>();
            m_midiIn->setCallback(&RtMidiController::trampoline, this);
            auto _in_name = QString::fromStdString(m_midiIn->getPortName(in_index));
            m_midiIn->openPort(in_index, in_name.toStdString());
            if(in_name.isEmpty()) {
                setDeviceName("RtMidi: " + in_name);
            }
            in_name = _in_name;
            setInputDevice(true);
        }catch(const RtMidiError &error) {
            qDebug() << error.what();
            in_index = -1;
            in_name.clear();
            setInputDevice(false);
        }
        if(_iindex != in_index)
            emit inputIndexChanged(in_index);
        if(_iname != in_name)
            emit inputNameChanged(in_name);
    }
    if(out_index>= 0) {
        auto _oindex = out_index;
        auto _oname  = out_name;
        try {
            m_midiOut = std::make_unique<RtMidiOut>();
            auto _out_name = QString::fromStdString(m_midiOut->getPortName(out_index));
            m_midiOut->openPort(out_index,out_name.toStdString());
            if(!out_name.isEmpty())
                setDeviceName("RtMidi: " + out_name);
            out_name = _out_name;
            setOutputDevice(true);
        }catch(const RtMidiError &error) {
            qDebug() << error.what();
            out_index = -1;
            out_name.clear();
            setOutputDevice(false);
        }
        if(_oindex != out_index)
            emit outputIndexChanged(out_index);
        if(_oname != out_name)
            emit outputNameChanged(out_name);

    }
    m_bInSysex = false;
    m_sysex.clear();;
    setOpen(true);
//    startEngine();
    return 0;
}

int RtMidiController::close()
{
    if (!isOpen()) {
        qDebug() << "RtMIDI device" << getDeviceName() << "already closed";
        return -1;
    }

//    stopEngine();
    MidiController::close();
    m_midiIn.reset();
    m_midiOut.reset();
    setOpen(false);
    return 0;
}

void RtMidiController::callback(double deltatime, std::vector<uint8_t> &message)
{
    auto timestamp = mixxx::Duration::fromNanos(qint64(deltatime * 1e9));
    if(message.size() < 1)
        return;
    auto status = message[0];
    if ((status & 0xF8) == 0xF8) {
        // Handle real-time MIDI messages at any time
        receive(status, 0, 0, timestamp);
        return;
    }
    if (!m_bInSysex) {
        if (status == 0xF0) {
            m_bInSysex = true;
            status = 0;
        } else {
            //unsigned char channel = status & 0x0F;
            if(message.size() < 3)
                return;
            auto note = message[1];
            auto velocity = message[2];
            receive(status, note, velocity, timestamp);
            return;
        }
    }
    if (m_bInSysex) {
        // Abort (drop) the current System Exclusive message if a
        //  non-realtime status byte was received
        if (status > 0x7F && status < 0xF7) {
            m_bInSysex = false;
            m_sysex.clear();
            qWarning() << "Buggy MIDI device: SysEx interrupted!";
            if (status == 0xF0) {
                m_bInSysex = true;
                status = 0;
            } else {
                //unsigned char channel = status & 0x0F;
                if(message.size() < 3)
                    return;
                auto note = message[1];
                auto velocity = message[2];
                receive(status, note, velocity, timestamp);
                return;
            }

        }
        // Collect bytes from PmMessage
        for(auto i = 1u; i < message.size() ; ++i) {
            auto data = message.at(i);
            // End System Exclusive message if the EOX byte was received
            if(data == MIDI_EOX) {
                receive( QByteArray::fromRawData(reinterpret_cast<const char *>(m_sysex.data()),m_sysex.size()),
                    timestamp);
                m_sysex.clear();
                m_bInSysex = false;
                return;
            }else{
                m_sysex.push_back(data);
            }
        }
    }
}
void RtMidiController::sendWord(unsigned int word)
{
    if(m_midiOut) {
        auto message = std::vector<uint8_t>{uint8_t(word),uint8_t(word>>8),uint8_t(word>>16)};
        m_midiOut->sendMessage(&message);
    }

}
void RtMidiController::send(QByteArray data)
{
    if(!m_midiOut)
        return;
    auto message = std::vector<uint8_t>(data.constBegin(),data.constEnd());
    m_midiOut->sendMessage(&message);
}
bool RtMidiController::poll(){return false;}
bool RtMidiController::isPolling() const { return false; }

void RtMidiController::trampoline(
    double deltatime
  , std::vector<uint8_t> * message
  , void *opaque)
{
    if(message && opaque)
        static_cast<RtMidiController*>(opaque)->callback(deltatime, *message);
}
