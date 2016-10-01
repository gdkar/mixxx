/**
 * @file midicontroller.cpp
 * @author Sean Pappalardo spappalardo@mixxx.org
 * @date Tue 7 Feb 2012
 * @brief MIDI Controller base class
 *
 */

#include "controllers/midi/midicontroller.h"

#include "controllers/midi/midiutils.h"
#include "controllers/defs_controllers.h"
#include "controllers/controllerdebug.h"
#include "control/controlobject.h"
#include "errordialoghandler.h"
#include "mixer/playermanager.h"
#include "util/math.h"

MidiController::MidiController(QObject *p)
        : Controller(p)
{
    setDeviceCategory(tr("MIDI Controller"));
}

MidiController::~MidiController() {
    // Don't close the device here. Sub-classes should close the device in their
    // destructors.
}
Q_INVOKABLE void MidiController::sendSysexMsg(QList<int> data, unsigned int length)
{
    send(data, length);
}

QString MidiController::presetExtension() const {
    return MIDI_PRESET_EXTENSION;
}

int MidiController::close() {
    return 0;
}

QString formatMidiMessage(const QString& controllerName,
                          unsigned char status, unsigned char control,
                          unsigned char value, unsigned char channel,
                          unsigned char opCode, mixxx::Duration timestamp) {
    switch (opCode) {
        case MIDI_PITCH_BEND:
            return QString("%1: t:%2 status 0x%3: pitch bend ch %4, value 0x%5")
                    .arg(controllerName, timestamp.formatMillisWithUnit(),
                         QString::number(status, 16).toUpper(),
                         QString::number(channel+1, 10),
                         QString::number((value << 7) | control, 16).toUpper().rightJustified(4,'0'));
        case MIDI_SONG_POS:
            return QString("%1: t:%5 status 0x%3: song position 0x%4")
                    .arg(controllerName, timestamp.formatMillisWithUnit(),
                         QString::number(status, 16).toUpper(),
                         QString::number((value << 7) | control, 16).toUpper().rightJustified(4,'0'));
        case MIDI_PROGRAM_CH:
        case MIDI_CH_AFTERTOUCH:
            return QString("%1: t:%2 status 0x%3 (ch %4, opcode 0x%5), value 0x%6")
                    .arg(controllerName, timestamp.formatMillisWithUnit(),
                         QString::number(status, 16).toUpper(),
                         QString::number(channel+1, 10),
                         QString::number((status & 255)>>4, 16).toUpper(),
                         QString::number(control, 16).toUpper().rightJustified(2,'0'));
        case MIDI_SONG:
            return QString("%1: t:%2 status 0x%3: select song #%4")
                    .arg(controllerName, timestamp.formatMillisWithUnit(),
                         QString::number(status, 16).toUpper(),
                         QString::number(control+1, 10));
        case MIDI_NOTE_OFF:
        case MIDI_NOTE_ON:
        case MIDI_AFTERTOUCH:
        case MIDI_CC:
            return QString("%1: t:%2 status 0x%3 (ch %4, opcode 0x%5), ctrl 0x%6, val 0x%7")
                    .arg(controllerName, timestamp.formatMillisWithUnit(),
                         QString::number(status, 16).toUpper(),
                         QString::number(channel+1, 10),
                         QString::number((status & 255)>>4, 16).toUpper(),
                         QString::number(control, 16).toUpper().rightJustified(2,'0'),
                         QString::number(value, 16).toUpper().rightJustified(2,'0'));
        default:
            return QString("%1: t:%2 status 0x%3")
                    .arg(controllerName, timestamp.formatMillisWithUnit(),
                         QString::number(status, 16).toUpper());
    }
}

void MidiController::receive(unsigned char status, unsigned char control,
                             unsigned char value, mixxx::Duration timestamp)
{

    {
        auto parts = MidiUtils::prefixFromMessage(status,control,value);
        auto pre = parts.first;
        auto val = parts.second;
        if(!pre.isEmpty()) {
            if(auto prox = m_dispatch.value(pre,nullptr)) {
                prox->setValue(val,timestamp.toDoubleSeconds());
            }
        }
    }
//    emit messageReceived(status,control,value,timestamp.toDoubleSeconds());
    auto channel = MidiUtils::channelFromStatus(status);
    auto opCode = MidiUtils::opCodeFromStatus(status);

    controllerDebug(formatMidiMessage(getDeviceName(), status, control, value,channel, opCode, timestamp));
}
double MidiController::computeValue(MidiOptions options, double _prevmidivalue, double _newmidivalue)
{
    double tempval = 0.;
    double diff = 0.;

    if (options.all == 0) {
        return _newmidivalue;
    }

    if (options.invert) {
        return 127. - _newmidivalue;
    }
    if (options.rot64 || options.rot64_inv) {
        tempval = _prevmidivalue;
        diff = _newmidivalue - 64.;
        if (diff == -1 || diff == 1)
            diff /= 16;
        else
            diff += (diff > 0 ? -1 : +1);
        if (options.rot64)
            tempval += diff;
        else
            tempval -= diff;
        return (tempval < 0. ? 0. : (tempval > 127. ? 127.0 : tempval));
    }

    if (options.rot64_fast) {
        tempval = _prevmidivalue;
        diff = _newmidivalue - 64.;
        diff *= 1.5;
        tempval += diff;
        return (tempval < 0. ? 0. : (tempval > 127. ? 127.0 : tempval));
    }

    if (options.diff) {
        //Interpret 7-bit signed value using two's compliment.
        if (_newmidivalue >= 64.)
            _newmidivalue = _newmidivalue - 128.;
        //Apply sensitivity to signed value. FIXME
       // if(sensitivity > 0)
        //    _newmidivalue = _newmidivalue * ((double)sensitivity / 50.);
        //Apply new value to current value.
        _newmidivalue = _prevmidivalue + _newmidivalue;
    }

    if (options.selectknob) {
        //Interpret 7-bit signed value using two's compliment.
        if (_newmidivalue >= 64.)
            _newmidivalue = _newmidivalue - 128.;
        //Apply sensitivity to signed value. FIXME
        //if(sensitivity > 0)
        //    _newmidivalue = _newmidivalue * ((double)sensitivity / 50.);
        //Since this is a selection knob, we do not want to inherit previous values.
    }

    if (options.button) {
        _newmidivalue = _newmidivalue != 0;
    }

    if (options.sw) {
        _newmidivalue = 1;
    }

    if (options.spread64) {
        //qDebug() << "MIDI_OPT_SPREAD64";
        // BJW: Spread64: Distance away from centre point (aka "relative CC")
        // Uses a similar non-linear scaling formula as ControlTTRotary::getValueFromWidget()
        // but with added sensitivity adjustment. This formula is still experimental.

        _newmidivalue = _newmidivalue - 64.;
        //FIXME
        //double distance = _newmidivalue - 64.;
        // _newmidivalue = distance * distance * sensitivity / 50000.;
        //if (distance < 0.)
        //    _newmidivalue = -_newmidivalue;

        //qDebug() << "Spread64: in " << distance << "  out " << _newmidivalue;
    }

    if (options.herc_jog) {
        if (_newmidivalue > 64.) {
            _newmidivalue -= 128.;
        }
        _newmidivalue += _prevmidivalue;
        //if (_prevmidivalue != 0.0) { qDebug() << "AAAAAAAAAAAA" << _prevmidivalue; }
    }

    if (options.herc_jog_fast) {
        if (_newmidivalue > 64.) {
            _newmidivalue -= 128.;
        }
        _newmidivalue = _prevmidivalue + (_newmidivalue * 3);
    }

    return _newmidivalue;
}

QString formatSysexMessage(const QString& controllerName, const QByteArray& data,
                           mixxx::Duration timestamp) {
    QString message = QString("%1: t:%2 %3 bytes: [")
            .arg(controllerName).arg(timestamp.formatMillisWithUnit())
            .arg(data.size());
    for (int i = 0; i < data.size(); ++i) {
        message += QString("%1%2").arg(
            QString("%1").arg((unsigned char)(data.at(i)), 2, 16, QChar('0')).toUpper(),
            QString("%1").arg((i < (data.size()-1)) ? ' ' : ']'));
    }
    return message;
}
void MidiController::sendShortMsg(unsigned int status, unsigned int byte1, unsigned int byte2)
{
    unsigned int word = (((unsigned int)byte2) << 16) | (((unsigned int)byte1) << 8) | status;
    sendWord(word);
}
