import org.mixxx.qml 0.1
import "./lib"
import QtQml 2.2
import QtQuick 2.7

Item {
    id: root
    property string group: "[Channel1]"
    property real wheel_multiplier: 0.4
    property var controller:parent.controller
    property var bindings
    Pushy     { parent:root;prefix:"\u0090" + bindings.fwd;item: "fwd";  }
    Toggle    { parent:root;prefix:"\u0090" + bindings.cue;item: "cue_default";  }
    Toggle    { parent:root;prefix:"\u0090" + bindings.play;item: "play"; }
    Toggle    { parent:root;prefix:"\u0090" + bindings.beatsync;item: "beatsync"; }
    Toggle    { parent:root;prefix:"\u0090" + bindings.pfl;item: "pfl"; }
    Pushy     { parent:root;prefix:"\u0090" + bindings.loadSelected;item: "LoadSelectedTrack"; }
    Pushy     { parent:root;prefix:"\u0090" + bindings.back;item: "back";  }
    BindValue { parent:root;prefix:"\u00b0" + bindings.rate;item: "rate";  }
    Toggle    { parent:root;prefix:"\u0090" + bindings.keylock;item: "keylock";}
    BindValue { parent:root;prefix:"\u00b0" + bindings.volume;item: "volume"; value: proxy.value / 128;}
    Pushy     { parent:root;prefix:"\u0090" + bindings.beat_trans_curpos;item: "beats_translate_curpos"; }
    Pushy     { parent:root;prefix:"\u0090" + bindings.quantize;item: "quantize" }
    Addy      { parent:root;prefix:"\u0090" + bindings.rate_perm_up;item:"rate";amount: 0.5e-2;min:-3;max:3;}
    Addy      { parent:root;prefix:"\u0090" + bindings.rate_perm_down;item:"rate";amount: -0.5e-2;min:-3;max:3;}
    BindValue { parent:root;prefix:"\u00b0" + bindings.filterLow;item: "filterLow"; value: proxy.value * 2 / 128.;}
    BindValue { parent:root;prefix:"\u00b0" + bindings.filterMid;item: "filterMid"; value: proxy.value * 2./ 128;}
    BindValue { parent:root;prefix:"\u00b0" + bindings.filterHigh;item: "filterHigh"; value: proxy.value * 2./ 128;}
    Jog       { parent:root;prefix:"\u00b0" + bindings.jog; item:"jog";}
    BeatLEDs  { id:beatLeds;parent:root;midi:root.controller;stepStart:bindings.stepStart;stepStop:bindings.stepStop}
    MidiOutput{ id:playLed;parent:root;midi:root.controller;item:"play";midiNumber:bindings.playLed;}
    MidiOutput{ id:loop_enabledLed;parent:root;midi:root.controller;item:"loop_enabled";midiNumber:bindings.loop_enabledLed;}
    MidiOutput{ id:beat_activeLed;parent:root;midi:root.controller;item:"beat_active";midiNumber:bindings.beat_activeLed;}
    MidiOutput{ id:keylockLed;parent:root;midi:root.controller;item:"keylock";midiNumber:bindings.keylockLed;}
    MidiOutput{ id:pflLed;parent:root;midi:root.controller;item:"pfl";midiNumber:bindings.pflLed;}
    MidiOutput{ id:quantizeLed;parent:root;midi:root.controller;item:"quantize";midiNumber:bindings.quantizeLed;}
}
