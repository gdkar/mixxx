import org.mixxx.qml 0.1
import "../qml"
import "../qml/keybindings"
import QtQml 2.2
import QtQuick 2.7
Item {
    id: root
    DeckControls {
        id: ch1
        group: "[Channel1]"
        bindings: { 'fwd':'\u0010',
                    'cue':'\u0011',
                    'play':'\u0012',
                    'beatsync':'\u0013',
                    'pfl':'\u0014',
                    'loadSelected':'\u0015',
                    'back':'\u000f',
                    'rate':'\u0034',
                    'keylock':'\u0004',
                    'volume':'\u0036',
                    'beat_trans_curpos':'\u0002',
                    'quantize':'\u0003',
                    'jog':'\u0030',
                    'stepStart':0x44,
                    'stepStop':0x47
        }
    }
    DeckControls {
        id: ch2
        group: "[Channel2]"
        bindings: { 'fwd':'\u0026',
                    'cue':'\u0027',
                    'play':'\u0028',
                    'beatsync':'\u0029',
                    'pfl':'\u002a',
                    'loadSelected':'\u002b',
                    'back':'\u002d',
                    'rate':'\u0035',
                    'keylock':'\u001a',
                    'volume':'\u003b',
                    'beat_trans_curpos':'\u0018',
                    'quantize':'\u0019',
                    'jog':'\u0031',
                    'stepStart':0x4c,
                    'stepStop':0x4f
        }
    }
    property Controller controller: RtMidiController {
        id: air
        inputIndex: -1
        outputIndex: -1
        Component.onCompleted: {
            for(var i = 0; i < air.inputPortCount;++i) {
                    if(String.prototype.indexOf(air.inputPortName(i), "DJ Control Air")>=0) {
                        air.in_index = i
                        break;
                    }
            }
            for(var i = 0; i < air.outputPortCount;++i) {
                    if(String.prototype.indexOf(air.outputPortName(i), "DJ Control Air")>=0) {
                        out_index = i;
                        break;
                    }
            }
            air.open()
            for(var i = 0; i < 79; ++i)
                air.sendShortMsg(0x90,i,0x00);
            air.sendShortMsg(0x90,0x3b,0x7f);air.sendShortMsg(0x90,0x3a,0x7f);
            console.log(JSON.stringify(air))
        }
    }
}
