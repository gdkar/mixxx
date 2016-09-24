import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

Item {
    id: root
    property string group: "[Channel1]"
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
    BeatLEDs  { id:beatLeds;parent:root;midi:root.controller;stepStart:bindings.stepStart;stepStop:bindings.stepStop}
}
