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
{ }

RtMidiController::RtMidiController(int inIndex, QString inName, int outIndex, QString outName, QObject *p)
        : MidiController(p),
          in_index(inIndex),
          in_name(inName),
          out_index(outIndex),
          out_name(outName)
{
    setInputDevice(in_index >= 0);
    setOutputDevice(out_index >= 0);
    if(!outName.isEmpty()) {
        setDeviceName("RtMidi: " + outName);
    }else if(!inName.isEmpty()) {
        setDeviceName("RtMidi: " + inName);
    }
}
int RtMidiController::inputIndex() const
{
    return in_index;
}
int RtMidiController::outputIndex() const
{
    return out_index;
}
QString RtMidiController::inputName() const
{
    return in_name;
}
QString RtMidiController::outputName() const
{
    return out_name;
}
void RtMidiController::setInputIndex(int _index)
{
    if(_index != in_index) {
        auto _open = isOpen();
        if(isOpen()){
            close();
        }
        in_index = _index;
        in_name.clear();
        emit(inputIndexChanged(_index));
        if(_open)
            open();
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
        qDebug() << "RtMidiDevice" << getName() << "already open";
        return -1;
    }else{
        qDebug() << "opening RtMidiDevice" << getName();
    }
    if(in_index>= 0) {
        try {
            m_midiIn = std::make_unique<RtMidiIn>();
            if(in_name.isEmpty()){
                in_name = QString::fromStdString(m_midiIn->getPortName(in_index));
                emit(inputNameChanged(in_name));
            }
            if(!in_name.isEmpty())
                setDeviceName("RtMidi: " + in_name);
            m_midiIn->setCallback(&RtMidiController::trampoline, this);
            m_midiIn->openPort(in_index, in_name.toStdString());
        }catch(const RtMidiError &error) {
            qDebug() << error.what();
        }
    }
    if(out_index>= 0) {
        try {
            m_midiOut = std::make_unique<RtMidiOut>();
            if(out_name.isEmpty()){
                out_name = QString::fromStdString(m_midiOut->getPortName(out_index));
                emit(outputNameChanged(out_name));
            }
            if(!out_name.isEmpty())
                setDeviceName("RtMidi: " + out_name);

            m_midiOut->openPort(out_index,out_name.toStdString());
        }catch(const RtMidiError &error) {
            qDebug() << error.what();
        }
    }

    m_bInSysex = false;
    m_sysex.clear();;
    setOpen(true);
    startEngine();
    return 0;
}

int RtMidiController::close()
{
    if (!isOpen()) {
        qDebug() << "RtMIDI device" << getName() << "already closed";
        return -1;
    }

    stopEngine();
    MidiController::close();
    m_midiIn.reset();
    m_midiOut.reset();
    setOpen(false);
    return 0;
}

void RtMidiController::callback(double deltatime, std::vector<unsigned char> &message)
{
    auto timestamp = mixxx::Duration::fromNanos(qint64(deltatime * 1e9));
    if(message.size() < 1)
        return;
    auto status = message[0];
    if ((status & 0xF8) == 0xF8) {
        // Handle real-time MIDI messages at any time
        QMetaObject::invokeMethod(this,"receive",Qt::QueuedConnection,Q_ARG(unsigned char, status), Q_ARG(unsigned char, 0), Q_ARG(unsigned char, 0), Q_ARG(mixxx::Duration,timestamp));
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

        QMetaObject::invokeMethod(this,"receive",Qt::QueuedConnection,Q_ARG(unsigned char, status), Q_ARG(unsigned char, note), Q_ARG(unsigned char, velocity), Q_ARG(mixxx::Duration,timestamp));
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
        QMetaObject::invokeMethod(this,"receive",Qt::QueuedConnection,Q_ARG(unsigned char, status), Q_ARG(unsigned char, note), Q_ARG(unsigned char, velocity), Q_ARG(mixxx::Duration,timestamp));
                return;
            }

        }
        // Collect bytes from PmMessage
        for(auto i = 1u; i < message.size() ; ++i) {
            auto data = message.at(i);
            // End System Exclusive message if the EOX byte was received
            if(data == MIDI_EOX) {
                QMetaObject::invokeMethod(this,"receive",Qt::QueuedConnection,
                    Q_ARG(QByteArray,QByteArray::fromRawData(reinterpret_cast<const char *>(m_sysex.data()), m_sysex.size())),Q_ARG(mixxx::Duration,timestamp));
                m_sysex.clear();
                m_bInSysex = false;
            }else{
                m_sysex.push_back(data);
            }
        }
    }
}
void RtMidiController::sendWord(unsigned int word)
{
    if(m_midiOut) {
        auto message = std::vector<unsigned char>{uint8_t(word),uint8_t(word>>8),uint8_t(word>>16)};
        m_midiOut->sendMessage(&message);
    }

}
void RtMidiController::send(QByteArray data)
{
    if(!m_midiOut)
        return;
    auto message = std::vector<unsigned char>(data.constBegin(),data.constEnd());
    m_midiOut->sendMessage(&message);
}

bool RtMidiController::poll(){return false;}
bool RtMidiController::isPolling() const { return false; }

void RtMidiController::trampoline(
    double deltatime
  , std::vector<unsigned char> * message
  , void *opaque)
{
    if(message && opaque)
        static_cast<RtMidiController*>(opaque)->callback(deltatime, *message);
}

