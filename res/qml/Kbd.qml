import org.mixxx.qml 0.1
import "./keybindings"
import QtQml 2.2
import QtQuick 2.7

Item {
    id: keyboardConfig
    Item {
        id: channel1
        property string group: "[Channel1]"
        ToggleKey { item: "play" ;      key: "d" }
        PushKey   { item: "cue_set";    key: "Shift+d"}
        PushKey   { item: "cue_default";key: "f" }
        PushKey   { item: "cue_gotoandstop";key:"Shift+f"}

        PushKey   { item: "back";key:"a"}
        PushKey   { item: "reverse";key:"Shift+a"}
        PushKey   { item: "fwd";key:"s"}

        PushKey   { item: "beatsync";key:"1"}
        PushKey   { item: "bpm_tap";key:"Shift+1"}
        PushKey   { item: "loop_in";key:"2"}
        PushKey   { item: "loop_out";key:"3"}
        PushKey   { item: "reloop_exit";key:"4"}
        Add       { item: "rate";amount: -0.5e-2;key:"F1";min:0;max:3}
        Add       { item: "rate";amount:  0.5e-2;key:"F2";min:0;max:3}
        Add       { item: "rate";amount: -0.1e-2;key:"Shift+F1";min:0;max:3}
        Add       { item: "rate";amount:  0.1e-2;key:"Shift+F2";min:0;max:3}
        IncDec    { item: "rate";amount: -2.e-2;key:"F3"}
        IncDec    { item: "rate";amount:  2.e-2;key:"F4"}
        IncDec    { item: "rate";amount: -0.5e-2;key:"Shift+F3"}
        IncDec    { item: "rate";amount:  0.5e-2;key:"Shift+F4"}

        ToggleKey { item: "pfl";key:"b"}
        Set       { item: "volume"; to: 1; key: "Ctrl+5"}
        Set       { item: "volume"; to: 0; key: "Ctrl+t"}
        Add       { item: "volume"; amount: 1/16; key:"5";min:0;max:1}
        Add       { item: "volume"; amount: -1/16; key:"t";min:0;max:1}
        Add       { item: "volume"; amount: 1/64; key:"Shift+%";min:0;max:1}
        Add       { item: "volume"; amount: -1/64; key:"Shift+t";min:0;max:1}

        PushKey   { item:"LoadSelectedTrack";key: "Shift+Left"}
        PushKey   { item:"eject";key:"Alt+Shift+Left"}
        PushKey   { item:"hotcue_1_activate";key:"z";}
        PushKey   { item:"hotcue_1_clear";key:"z";}
        PushKey   { item:"hotcue_2_activate";key:"x";}
        PushKey   { item:"hotcue_2_clear";key:"Shift+x";}
        PushKey   { item:"hotcue_3_activate";key:"c";}
        PushKey   { item:"hotcue_3_clear";key:"Shift+c";}
        PushKey   { item:"hotcue_4_activate";key:"v";}
        PushKey   { item:"hotcue_4_clear";key:"Shift+v";}

    }
    Item {
        id: channel2
        property string group: "[Channel2]"
        ToggleKey { item: "play" ;      key: "l" }
        PushKey   { item: "cue_set";    key: "Shift+l"}
        PushKey   { item: "cue_default";key: ";" }
        PushKey   { item: "cue_gotoandstop";key:"Shift+:"}

        PushKey   { item: "back";key:"j"}
        PushKey   { item: "reverse";key:"Shift+j"}
        PushKey   { item: "fwd";key:"k"}

        PushKey   { item: "beatsync";key:"-"}
        PushKey   { item: "bpm_tap";key:"Shift+_"}
        PushKey   { item: "loop_in";key:"7"}
        PushKey   { item: "loop_out";key:"8"}
        PushKey   { item: "reloop_exit";key:"9"}

        Add       { item: "rate";amount: -0.5e-2;key:"F5";min:0;max:3}
        Add       { item: "rate";amount:  0.5e-2;key:"F6";min:0;max:3}
        Add       { item: "rate";amount: -0.1e-2;key:"Shift+F5";min:0;max:3}
        Add       { item: "rate";amount:  0.1e-2;key:"Shift+F6";min:0;max:3}
        IncDec    { item: "rate";amount: -2.e-2;key:"F7"}
        IncDec    { item: "rate";amount:  2.e-2;key:"F8"}
        IncDec    { item: "rate";amount: -0.5e-2;key:"Shift+F7"}
        IncDec    { item: "rate";amount:  0.5e-2;key:"Shift+F8"}

        ToggleKey { item: "pfl";key:"n"}

        Set       { item: "volume"; to: 1; key: "Ctrl+6"}
        Set       { item: "volume"; to: 0; key: "Ctrl+y"}
        Add       { item: "volume"; amount: 1/16; key:"6";min:0;max:1}
        Add       { item: "volume"; amount: -1/16; key:"^";min:0;max:1}
        Add       { item: "volume"; amount: 1/64; key:"Shift+y";min:0;max:1}
        Add       { item: "volume"; amount: -1/64; key:"Shift+y";min:0;max:1}
        PushKey   { item:"LoadSelectedTrack";key: "Shift+Right";}
        PushKey   { item:"eject";key:"Alt+Shift+Right"}
        PushKey   { item:"hotcue_1_activate";key:"m";}
        PushKey   { item:"hotcue_1_clear";key:"Shift+m";}
        PushKey   { item:"hotcue_2_activate";key:",";}
        PushKey   { item:"hotcue_2_clear";key:"Shift+<";}
        PushKey   { item:"hotcue_3_activate";key:".";}
        PushKey   { item:"hotcue_3_clear";key:"Shift+>";}
        PushKey   { item:"hotcue_4_activate";key:"/";}
        PushKey   { item:"hotcue_4_clear";key:"Shift+?";}
    }
    Item {
        id: master
        property string group: "[Master]"
        Set       { item: "crossfader";to: -1;key:"Ctrl+g"}
        Set       { item: "crossfader";to:  1;key:"Ctrl+h"}
        Add       { item: "crossfader";amount: -1/16;key:"g";min:-1;max:1}
        Add       { item: "crossfader";amount:  1/16;key:"h";min:-1;max:1}
        Add       { item: "crossfader";amount: -1/8;key:"Shift+g";min:-1;max:1}
        Add       { item: "crossfader";amount:  1/8;key:"Shift+h";min:-1;max:1}

    }
    Item {
        id: keyboardShortcuts
        property string group: "[KeyboardShortcuts]"
        PushKey { item: "FileMenu_LoadDeck1";key:"Ctrl+o"}
        PushKey { item: "FileMenu_LoadDeck2";key:"Ctrl+Shift+o"}
        PushKey { item: "FileMenu_Quit";key:"Ctrl+q"}
        PushKey { item: "ViewMenu_MaximizeLibrary";key:"Ctrl+Space"}
        PushKey { item: "OptionsMenu_EnableLiveBroadcasting";key:"Ctrl+l"}
        PushKey { item: "OptionsMenu_RecordMix";key:"Ctrl+r"}
        PushKey { item: "OptionsMenu_ReloadSkin";key:"Ctrl+Shift+r"}

    }
}
