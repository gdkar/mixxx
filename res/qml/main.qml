import org.mixxx.qml 0.1
import QtQuick 2.6
import QtQuick.Extras 1.4
import QtQuick.Window 2.2
import QtQuick.Controls 1.5

Item {
    id: root
    Deck {
        id: channel1
        group: "[Channel1]"
        controls: ["rate","volume","playposition","jog","play","playing"]
    }
    Deck {
        id: channel2
        group: "[Channel2]"
        controls: ["rate","volume","playposition","jog","play","playing"]
    }
    Pushy     { id: load1;prefix:"\u0090\u0015";group : "[Channel1]";item: "LoadSelectedTrack"; }
    Toggle    { id: pfl1;prefix:"\u0090\u0014";group : "[Channel1]";item: "pfl"; }
    Toggle    { id: sync1;prefix:"\u0090\u0013";group : "[Channel1]";item: "beatsync"; }
    Toggle    { id: play1;prefix:"\u0090\u0012";group : "[Channel1]";item: "play"; }
    Toggle    { id: cue1;prefix:"\u0090\u0011"; group : "[Channel1]";item: "cue";  }
    Pushy     { id: fwd1;prefix:"\u0090\u0010"; group : "[Channel1]";item: "fwd";  }
    Pushy     { id: back1;prefix:"\u0090\u000f"; group : "[Channel1]";item: "back";  }
    BindValue { id:rate1;prefix:"\u00b0\u0034"; group : "[Channel1]";item: "rate";  }
    BindValue {
        id:xfade
        prefix:"\u00b0\u003a"
        group:"[Master]"
        item:"crossfader"
    }
    BindValue {
        id:vol1
        prefix:"\u00b0\u0036"
        group : "[Channel1]"
        item: "volume";
        transformed: proxy.value / 128
    }
    Pushy {
        id:beat_align1
        prefix:"\u0090\u0002";
        group:"[Channel1]";
        item:"beat_translate_curpos";
    }
    Pushy {
        id:quantize1
        prefix:"\u0090\u0003"
        group:"[Channel1]"
        item:"quantize"
    }
    Pushy {
        id:keylock1
        prefix:"\u0090\u0004"
        group:"[Channel1]"
        item:"keylock"
    }

    Pushy     { id: beat_align2;prefix:"\u0090\u0018";group : "[Channel2]";item: "beat_translate_curpos"; }
    Toggle    { id: quantize2;prefix:"\u0090\u0019";group : "[Channel2]";item: "quantize"; }
    Toggle    { id:play2;prefix:"\u0090(";      group : "[Channel2]";item: "play"; }
    Toggle    { id:cue2;prefix:"\u0090'";       group : "[Channel2]";item: "cue";  }
    BindValue { id:rate2;prefix:"\u00b0\u0035"; group : "[Channel2]";item: "rate";  }
    BindValue {
        id:vol2
        prefix:"\u00b0\u003b"
        group : "[Channel2]"
        item: "volume";
        transformed: proxy.value / 128
    }

    property var controller: RtMidiController {
        id: air
        inputIndex: 1
        outputIndex: -1
        Component.onCompleted: {
            air.open()
        }
    }
}
