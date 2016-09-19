#ifndef MIDIUTILS_H
#define MIDIUTILS_H

#include "controllers/midi/midimessage.h"

class MidiUtils {
  public:
    static constexpr unsigned char channelFromStatus(uint8_t status) {
        return status & 0x0F;
    }

    static MidiOpCode constexpr opCodeFromStatus(uint8_t status) {
        auto opCode = status & 0xF0u;
        // MIDI_SYSEX and higher don't have a channel and occupy the entire byte.
        if (opCode == 0xF0u)
            opCode = status;
        return static_cast<MidiOpCode>(opCode);
    }
    static std::pair<QString,double> prefixFromMessage(uint8_t status, uint8_t control, uint8_t value)
    {
        auto op = opCodeFromStatus(status);
        QChar strbuf[3] = { status, control, value};
        auto prefix = QString{};
        auto val  = 0.;
        switch(op>>4) {
            case 0x8:
            case 0x9: {
                if(value==0)
                    status = 0x80;
            }
            /* fallthrough intended */
            case 0xa:
            case 0xb:
                prefix = QString(strbuf, 2);
                val = value ;
                break;
            case 0xc:
            case 0xd:
                prefix = QString(strbuf, 1);
                val = control;
                break;
            case 0xe:
                prefix = QString(strbuf, 2);
                val = (((uint32_t(value)<<7)|uint32_t(control)))/128.;
                break;
            default:
                break;
        }
        return std::make_pair(prefix, val);
    }
    static bool isMessageTwoBytes(uint8_t opCode) {
        switch (opCode) {
            case MIDI_SONG:
            case MIDI_NOTE_OFF:
            case MIDI_NOTE_ON:
            case MIDI_AFTERTOUCH:
            case MIDI_CC: return true;
            default:      return false;
        }
    }

    static bool isClockSignal(const MidiKey& mappingKey) {
        return (mappingKey.key & MIDI_TIMING_CLK) == MIDI_TIMING_CLK;
    }

    static QString opCodeToTranslatedString(MidiOpCode code);
    static QString formatByteAsHex(unsigned char value);
    static QString midiOptionToTranslatedString(MidiOption option);
};


#endif /* MIDIUTILS_H */
