import org.mixxx.qml 0.1
import "../qml"
import "../qml/keybindings"
import "./hercules_air"

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
                    'stepStop':0x47,
                    'rate_perm_up':'\u000e',
                    'rate_perm_down':'\u000d',
                    'filterLow':'\u0039',
                    'filterMid':'\u0038',
                    'filterHigh':'\u0037',

                    'playLed':0x12,
                    'keylockLed':0x04,
                    'loop_enabledLed':0x0c,
                    'beat_activeLed':0x13,
                    'pflLed':0x14,
                    'quantizeLed':0x03

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
                    'stepStop':0x4f,
                    'rate_perm_up':'\u0024',
                    'rate_perm_down':'\u0023',
                    'filterLow':'\u003e',
                    'filterMid':'\u003d',
                    'filterHigh':'\u003c',
                    'playLed':0x28,
                    'keylockLed':0x1a,
                    'loop_enabledLed':0x22,
                    'beat_activeLed':0x29,
                    'pflLed':0x2a,
                    'quantizeLed':0x19
        }
    }
    PlaylistControls {
        id: playlist
        group: "[Playlist]"
        bindings: {
            'next': '\u0034',
            'prev': '\u0033',
            'nextList':'\u0035',
            'prevList':'\u0036'
            }
    }
    property bool shift: (shiftButton.value != 0)
    property BindProxy shiftButton: controller.getBindingFor("\u0090\u002e")
    property Controller controller: RtMidiController {
        id: air
        inputIndex: 1
        outputIndex: 1
        Component.onCompleted: {
            air.close()
            for(var i = 0; i < air.inputPortCount;++i) {
                console.log(air.inputPortName(i))
                    if(String.prototype.indexOf("DJ",air.inputPortName(i) )>=0) {
                        air.inputIndex=
                        console.log("****")
                    }
            }
            for(var i = 0; i < air.outputPortCount;++i) {
                console.log(air.outputPortName(i))
                    if(String.prototype.indexOf("DJ",air.outputPortName(i))>=0) {
                        air.outputIndex= i;
                        console.log("****")
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
