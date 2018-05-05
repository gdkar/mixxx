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
RtMidiControllerInfo::~RtMidiControllerInfo() = default;
RtMidiControllerInfo::RtMidiControllerInfo(const QString& iname, const QString &oname, int iidx, int oidx)
: inputName{iname}
, outputName{oname}
, inputIndex{iidx}
, outputIndex{oidx}
{}
namespace {
bool namesMatchMidiPattern(const QString input_name,
                           const QString output_name) {
    // Some platforms format MIDI device names as "deviceName MIDI ###" where
    // ### is the instance # of the device. Therefore we want to link two
    // devices that have an equivalent "deviceName" and ### section.
    QRegExp deviceNamePattern("^(.*) MIDI (\\d+)( .*)?$");

    auto inputMatch = deviceNamePattern.indexIn(input_name);
    if (inputMatch == 0) {
        auto inputDeviceName = deviceNamePattern.cap(1);
        auto inputDeviceIndex = deviceNamePattern.cap(2);
        auto outputMatch = deviceNamePattern.indexIn(output_name);
        if (outputMatch == 0) {
            auto outputDeviceName = deviceNamePattern.cap(1);
            auto outputDeviceIndex = deviceNamePattern.cap(2);
            if (outputDeviceName.compare(inputDeviceName, Qt::CaseInsensitive) == 0 &&
                outputDeviceIndex == inputDeviceIndex) {
                return true;
            }
        }
    }
    return false;
}
bool namesMatchPattern(const QString input_name,
                       const QString output_name) {
    // This is a broad pattern that matches a text blob followed by a numeral
    // potentially followed by non-numeric text. The non-numeric requirement is
    // meant to avoid corner cases around devices with names like "Hercules RMX
    // 2" where we would potentially confuse the number in the device name as
    // the ordinal index of the device.
    QRegExp deviceNamePattern("^(.*) (\\d+)( [^0-9]+)?$");

    auto inputMatch = deviceNamePattern.indexIn(input_name);
    if (inputMatch == 0) {
        auto inputDeviceName = deviceNamePattern.cap(1);
        auto inputDeviceIndex = deviceNamePattern.cap(2);
        auto outputMatch = deviceNamePattern.indexIn(output_name);
        if (outputMatch == 0) {
            auto outputDeviceName = deviceNamePattern.cap(1);
            auto outputDeviceIndex = deviceNamePattern.cap(2);
            if (outputDeviceName.compare(inputDeviceName, Qt::CaseInsensitive) == 0 &&
                outputDeviceIndex == inputDeviceIndex) {
                return true;
            }
        }
    }
    return false;
}

bool namesMatchInOutPattern(const QString input_name,
                            const QString output_name) {
    QString basePattern = "^(.*) %1 (\\d+)( .*)?$";
    QRegExp inputPattern(basePattern.arg("in"));
    QRegExp outputPattern(basePattern.arg("out"));

    int inputMatch = inputPattern.indexIn(input_name);
    if (inputMatch == 0) {
        auto inputDeviceName = inputPattern.cap(1);
        auto inputDeviceIndex = inputPattern.cap(2);
        int outputMatch = outputPattern.indexIn(output_name);
        if (outputMatch == 0) {
            auto outputDeviceName = outputPattern.cap(1);
            auto outputDeviceIndex = outputPattern.cap(2);
            if (outputDeviceName.compare(inputDeviceName, Qt::CaseInsensitive) == 0 &&
                outputDeviceIndex == inputDeviceIndex) {
                return true;
            }
        }
    }
    return false;
}


bool shouldLinkInputToOutput(const QString input_name,
                             const QString output_name)
{
    // Early exit.
    if (input_name == output_name)
        return true;
    // Some device drivers prepend "To" and "From" to the names of their MIDI
    // ports. If the output and input device names don't match, let's try
    // trimming those words from the start, and seeing if they then match.

    // Ignore "From" text in the beginning of device input name.
    auto input_name_stripped = input_name;
    if (input_name.indexOf("from", 0, Qt::CaseInsensitive) == 0) {
        input_name_stripped = input_name.right(input_name.length() - 4);
    }

    // Ignore "To" text in the beginning of device output name.
    auto output_name_stripped = output_name;
    if (output_name.indexOf("to", 0, Qt::CaseInsensitive) == 0) {
        output_name_stripped = output_name.right(output_name.length() - 2);
    }

    if (output_name_stripped != input_name_stripped) {
        // Ignore " input " text in the device names
        auto offset = input_name_stripped.indexOf(" input ", 0,
                                                 Qt::CaseInsensitive);
        if (offset != -1) {
            input_name_stripped = input_name_stripped.replace(offset, 7, " ");
        }
        // Ignore " output " text in the device names
        offset = output_name_stripped.indexOf(" output ", 0,Qt::CaseInsensitive);
        if (offset != -1) {
            output_name_stripped = output_name_stripped.replace(offset, 8, " ");
        }
    }
    if (input_name_stripped == output_name_stripped ||
        namesMatchMidiPattern(input_name_stripped, output_name_stripped) ||
        namesMatchMidiPattern(input_name, output_name) ||
        namesMatchInOutPattern(input_name_stripped, output_name_stripped) ||
        namesMatchInOutPattern(input_name, output_name) ||
        namesMatchPattern(input_name_stripped, output_name_stripped) ||
        namesMatchPattern(input_name, output_name)) {
        return true;
    }

    return false;
}

}
QList<RtMidiControllerInfo> RtMidiController::deviceList() const
{
    auto retval = QList<RtMidiControllerInfo>{};
    try {
        auto inMap = QMap<QString,int>{};
        for(auto i = 0; i < inputPortCount();++i){
            inMap.insert(inputPortName(i),i);
        }
        for(auto i = 0; i < outputPortCount();++i){
            auto oname = outputPortName(i);
            auto found = false;
            for(auto it = inMap.begin(); it != inMap.end();++it){
                if(shouldLinkInputToOutput(it.key(), oname)) {
                    retval.append(RtMidiControllerInfo{
                        it.key(),
                        oname,
                        it.value(),
                        i});
                    inMap.erase(it);
                    found = true;
                    break;
                }
            }
            if(!found) {
                retval.append(RtMidiControllerInfo{
                    QString{},
                    oname,
                    -1,
                     i});
            }
        }
        for(auto it = inMap.cbegin(); it != inMap.cend(); ++it){
            retval.append(RtMidiControllerInfo{
                it.key(),
                QString{},
                it.value(),
                -1});
        }
    } catch(...) {
        return retval;
    }
    return retval;
}
RtMidiController::RtMidiController(QObject *p)
: MidiController()
, m_midiIn(std::make_unique<RtMidiIn>())
, m_midiOut(std::make_unique<RtMidiOut>())

{
}

RtMidiController::RtMidiController(int inIndex, QString inName, int outIndex, QString outName, QObject *p)
        : MidiController(),
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
QStringList RtMidiController::outputPortNames() const
{
    auto retval = QStringList{};
    for (auto i = 0; i < outputPortCount(); ++i) {
        retval << outputPortName(i);
    }
    return retval;
}
QStringList RtMidiController::inputPortNames() const
{
    auto retval = QStringList{};
    for (auto i = 0; i < inputPortCount(); ++i) {
        retval << inputPortName(i);
    }
    return retval;
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
        qDebug() << "RtMidiDevice" << getName() << "already open";
        return -1;
    }else{
        qDebug() << "opening RtMidiDevice" << getName();
    }
    try {
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
        startEngine();
    } catch(...) {
        qWarning() << "Something went wrong.";
    }
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

void RtMidiController::callback(double deltatime, std::vector<uint8_t> &message)
{
    auto timestamp = mixxx::Duration::fromNanos(qint64(deltatime * 1e9));
    if(message.size() < 1)
        return;
    auto status = message[0];
    try {
        if ((status & 0xF8) == 0xF8) {
            // Handle real-time MIDI messages at any time
            QMetaObject::invokeMethod(this,"receive", Q_ARG(unsigned char, status),Q_ARG(unsigned char, 0), Q_ARG(unsigned char, 0), Q_ARG(mixxx::Duration, timestamp));
    //        receive(status, 0, 0, timestamp);
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

                QMetaObject::invokeMethod(this,"receive", Q_ARG(unsigned char, status),Q_ARG(unsigned char, note), Q_ARG(unsigned char, velocity), Q_ARG(mixxx::Duration, timestamp));
    //            receive(status, note, velocity, timestamp);
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

                    QMetaObject::invokeMethod(this,"receive", Q_ARG(unsigned char, status),Q_ARG(unsigned char, note), Q_ARG(unsigned char, velocity), Q_ARG(mixxx::Duration, timestamp));
                    receive(status, note, velocity, timestamp);
                    return;
                }

            }
            // Collect bytes from PmMessage
            for(auto i = 1u; i < message.size() ; ++i) {
                auto data = message.at(i);
                // End System Exclusive message if the EOX byte was received
                if(data == MIDI_EOX) {
                    QMetaObject::invokeMethod(this,"receive", Q_ARG(QByteArray,QByteArray::fromRawData(reinterpret_cast<const char*>(m_sysex.data()),m_sysex.size())),Q_ARG(mixxx::Duration, timestamp));
    //                receive( QByteArray::fromRawData(reinterpret_cast<const char *>(m_sysex.data()),m_sysex.size()),
    //                    timestamp);
                    m_sysex.clear();
                    m_bInSysex = false;
                    return;
                }else{
                    m_sysex.push_back(data);
                }
            }
        }
    }
    catch(const RtMidiError & e) {
        qWarning() << "RtMidiError in callback:" << e.what();
    }
    catch(...) {
        qWarning() << "Unknown error in callback";
    }
}
void RtMidiController::sendShortMsg(unsigned char w0, unsigned char w1, unsigned char w2)
{
    if(m_midiOut) {
        try{
        auto message = std::vector<uint8_t>{ w0, w1, w2 };
//        std::vector<uint8_t>{uint8_t(word),uint8_t(word>>8),uint8_t(word>>16)};
        m_midiOut->sendMessage(&message);
        }
        catch(const RtMidiError & e) {
            qWarning() << "RtMidiError in callback:" << e.what();
        }
        catch(...) {
            qWarning() << "Unknown error in callback";
        }

    }

}
void RtMidiController::send(QByteArray data)
{
    if(!m_midiOut)
        return;
    auto message = std::vector<uint8_t>(data.constBegin(),data.constEnd());
    try {
        m_midiOut->sendMessage(&message);
    }
    catch(const RtMidiError & e) {
        qWarning() << "RtMidiError in callback:" << e.what();
    }
    catch(...) {
        qWarning() << "Unknown error in callback";
    }

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
