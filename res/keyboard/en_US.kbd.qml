import org.mixxx.qml 0.1
import './keybindings'
import QtQml 2.2
import QtQuick 2.7

Item {
    id: keyboardConfig
    Item {
        id: channel1
        property string group: '[Channel1]'
        ToggleKey { item: 'play' ;      seq: 'd' }
        PushKey   { item: 'cue_set';    seq: 'Shift+d'}
        PushKey   { item: 'cue_default';seq: 'f' }
        PushKey   { item: 'cue_gotoandstop';seq:'Shift+f'}

        PushKey   { item: 'back';seq:'a'}
        PushKey   { item: 'reverse';seq:'Shift+a'}
        PushKey   { item: 'fwd';seq:'s'}

        PushKey   { item: 'beatsync';seq:'1'}
        ToggleKey { item: 'keylock';seq:'Shift+!'}
        PushKey   { item: 'loop_in';seq:'2'}
        PushKey   { item: 'loop_out';seq:'3'}
        PushKey   { item: 'reloop_exit';seq:'4'}
        AddKey    { item: 'rate';amount: -0.5e-2;seq:'F1';min:-3;max:3}
        AddKey    { item: 'rate';amount:  0.5e-2;seq:'F2';min:-3;max:3}
        AddKey    { item: 'rate';amount: -0.1e-2;seq:'Shift+F1';min:-3;max:3}
        AddKey    { item: 'rate';amount:  0.1e-2;seq:'Shift+F2';min:-3;max:3}
        IncDec    { item: 'rate';amount: -2.e-2;seq:'F3'}
        IncDec    { item: 'rate';amount:  2.e-2;seq:'F4'}
        IncDec    { item: 'rate';amount: -0.5e-2;seq:'Shift+F3'}
        IncDec    { item: 'rate';amount:  0.5e-2;seq:'Shift+F4'}

        ToggleKey { item: 'pfl';seq:'b'}
        SetKey    { item: 'volume'; to: 1; seq: 'Ctrl+5'}
        SetKey    { item: 'volume'; to: 0; seq: 'Ctrl+t'}
        AddKey    { item: 'volume'; amount: 1/16; seq:'5';min:0;max:1}
        AddKey    { item: 'volume'; amount: -1/16; seq:'t';min:0;max:1}
        AddKey    { item: 'volume'; amount: 1/64; seq:'Shift+%';min:0;max:1}
        AddKey    { item: 'volume'; amount: -1/64; seq:'Shift+t';min:0;max:1}

        SetKey    { item: 'filterHigh';to: 0;seq:'Alt+Shift+#'}
        SetKey    { item: 'filterHigh';to: 1;seq:'Alt+Shift+$'}
        AddKey    { item: 'filterHigh';amount: -1/8;seq:'Alt+3';min:0;max:4}
        AddKey    { item: 'filterHigh';amount:  1/8;seq:'Alt+4';min:0;max:4}

        SetKey    { item: 'filterMid';to: 0;seq:'Alt+Shift+e'}
        SetKey    { item: 'filterMid';to: 1;seq:'Alt+Shift+r'}
        AddKey    { item: 'filterMid';amount: -1/8;seq:'Alt+e';min:0;max:4}
        AddKey    { item: 'filterMid';amount:  1/8;seq:'Alt+r';min:0;max:4}

        SetKey    { item: 'filterLow';to: 0;seq:'Alt+Shift+d'}
        SetKey    { item: 'filterLow';to: 1;seq:'Alt+Shift+f'}
        AddKey    { item: 'filterLow';amount: -1/8;seq:'Alt+d';min:0;max:4}
        AddKey    { item: 'filterLow';amount:  1/8;seq:'Alt+f';min:0;max:4}


        PushKey   { item:'LoadSelectedTrack';seq: 'Shift+Left'}
        PushKey   { item:'eject';seq:'Alt+Shift+Left'}

/*        PushKey   { item:'hotcue_1_activate';seq:'z';}
        PushKey   { item:'hotcue_1_clear';seq:'z';}
        PushKey   { item:'hotcue_2_activate';seq:'x';}
        PushKey   { item:'hotcue_2_clear';seq:'Shift+x';}*/
    }
    Item {
        id: channel2
        property string group: '[Channel2]'
        ToggleKey { item: 'play' ;      seq: 'l' }
        PushKey   { item: 'cue_set';    seq: 'Shift+l'}
        PushKey   { item: 'cue_default';seq: ';' }
        PushKey   { item: 'cue_gotoandstop';seq:'Shift+:'}

        PushKey   { item: 'back';seq:'j'}
        PushKey   { item: 'reverse';seq:'Shift+j'}
        PushKey   { item: 'fwd';seq:'k'}

        PushKey   { item: 'beatsync';seq:'-'}
        ToggleKey   { item: 'keylock';seq:'Shift+_'}
        PushKey   { item: 'loop_in';seq:'7'}
        PushKey   { item: 'loop_out';seq:'8'}
        PushKey   { item: 'reloop_exit';seq:'9'}

        AddKey    { item: 'rate';amount: -0.5e-2;seq:'F5';min:-3;max:3}
        AddKey    { item: 'rate';amount:  0.5e-2;seq:'F6';min:-3;max:3}
        AddKey    { item: 'rate';amount: -0.1e-2;seq:'Shift+F5';min:-3;max:3}
        AddKey    { item: 'rate';amount:  0.1e-2;seq:'Shift+F6';min:-3;max:3}
        IncDec    { item: 'rate';amount: -2.e-2;seq:'F7'}
        IncDec    { item: 'rate';amount:  2.e-2;seq:'F8'}
        IncDec    { item: 'rate';amount: -0.5e-2;seq:'Shift+F7'}
        IncDec    { item: 'rate';amount:  0.5e-2;seq:'Shift+F8'}

        ToggleKey { item: 'pfl';seq:'n'}

        SetKey    { item: 'volume'; to: 1; seq: 'Ctrl+6'}
        SetKey    { item: 'volume'; to: 0; seq: 'Ctrl+y'}
        AddKey    { item: 'volume'; amount: 1/16.; seq:'6';min:0;max:1}
        AddKey    { item: 'volume'; amount: -1/16.; seq:'y';min:0;max:1}
        AddKey    { item: 'volume'; amount: 1/64; seq:'Shift+^';min:0;max:1}
        AddKey    { item: 'volume'; amount: -1/64; seq:'Shift+y';min:0;max:1}

        AddKey    { item: 'filterHigh';amount: -1/8;seq:'Alt+7';min:0;max:4}
        AddKey    { item: 'filterHigh';amount:  1/8;seq:'Alt+8';min:0;max:4}
        SetKey    { item: 'filterHigh';to: 0;seq:'Alt+Shift+&'}
        SetKey    { item: 'filterHigh';to: 1;seq:'Alt+Shift+*'}

        AddKey    { item: 'filterMid';amount: -1/8;seq:'Alt+u';min:0;max:4}
        AddKey    { item: 'filterMid';amount:  1/8;seq:'Alt+i';min:0;max:4}
        SetKey    { item: 'filterMid';to: 0;seq:'ALt+Shift+u'}
        SetKey    { item: 'filterMid';to: 1;seq:'Alt+Shift+i'}

        AddKey    { item: 'filterLow';amount: -1/8;seq:'Alt+j'}
        AddKey    { item: 'filterLow';amount:  1/8;seq:'Alt+k'}
        SetKey    { item: 'filterLow';to:0;seq:'Alt+Shift+j'}
        SetKey    { item: 'filterLow';to:1;seq:'Alt+Shift+k'}


        PushKey   { item:'LoadSelectedTrack';seq: 'Shift+Right';}
        PushKey   { item:'eject';seq:'Alt+Shift+Right'}

/*        PushKey   { item:'hotcue_1_activate';seq:'m';}
        PushKey   { item:'hotcue_1_clear';seq:'Shift+m';}
        PushKey   { item:'hotcue_2_activate';seq:',';}
        PushKey   { item:'hotcue_2_clear';seq:'Shift+<';}*/
    }
    Item {
        id: master
        property string group: '[Master]'
        SetKey    { item: 'crossfader';to: -1;seq:'Ctrl+g'}
        SetKey    { item: 'crossfader';to:  1;seq:'Ctrl+h'}
        AddKey    { item: 'crossfader';amount: -1/16;seq:'g';min:-1;max:1}
        AddKey    { item: 'crossfader';amount:  1/16;seq:'h';min:-1;max:1}
        AddKey    { item: 'crossfader';amount: -1/8;seq:'Shift+g';min:-1;max:1}
        AddKey    { item: 'crossfader';amount:  1/8;seq:'Shift+h';min:-1;max:1}

    }
    Item {
        id: keyboardShortcuts
        property string group: '[KeyboardShortcuts]'
//c        PushKey { item: 'FileMenu_LoadDeck1';seq:'Ctrl+o'}
//        PushKey { item: 'FileMenu_LoadDeck2';seq:'Ctrl+Shift+o'}
        PushKey { item: 'FileMenu_Quit';seq:'Ctrl+q'}
//        PushKey { item: 'ViewMenu_MaximizeLibrary';seq:'Ctrl+Space'}
//        PushKey { item: 'OptionsMenu_EnableLiveBroadcasting';seq:'Ctrl+l'}
//        PushKey { item: 'OptionsMenu_RecordMix';seq:'Ctrl+r'}
//        PushKey { item: 'OptionsMenu_ReloadSkin';seq:'Ctrl+Shift+r'}
    }
    Component.onCompleted: {
        for(var i = 0; i < keyboardConfig.data.length;++i) {
            var sub = keyboardConfig.data[i];
            for(var j = 0; j < sub.data.length; ++j) {
                var ssub = sub.data[j];
                console.log(ssub.group,ssub.item,ssub.seq)
            }
        }
    }
}
