import org.mixxx.qml 0.1
import QtQuick 2.7
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
/*    ApplicationWindow{
        id: consoleWindow
        width: 640
        height: 480
        visible: true
        Console {
            id: consoleItem
            anchors.fill: parent
        }
    }*/
    Pushy     { source:air;id: load1;prefix:"\u0090\u0015"; group : "[Channel1]";item: "LoadSelectedTrack"; }
    Toggle    { source:air;id: pfl1; prefix:"\u0090\u0014"; group : "[Channel1]";item: "pfl"; }
    Toggle    { source:air;id: sync1;prefix:"\u0090\u0013"; group : "[Channel1]";item: "beatsync"; }
    Toggle    { source:air;id: play1;prefix:"\u0090\u0012"; group : "[Channel1]";item: "play"; }
    Toggle    { source:air;id: cue1; prefix:"\u0090\u0011"; group : "[Channel1]";item: "cue";  }
    Pushy     { source:air;id: fwd1; prefix:"\u0090\u0010"; group : "[Channel1]";item: "fwd";  }
    Pushy     { source:air;id: back1;prefix:"\u0090\u000f"; group : "[Channel1]";item: "back";  }
    BindValue { source:air;id:rate1; prefix:"\u00b0\u0034"; group : "[Channel1]";item: "rate";  }
    BindValue {
        source:air;id:xfade
        prefix:"\u00b0\u003a"
        group:"[Master]"
        item:"crossfader"
    }
    BindValue {
        source:air;id:vol1
        prefix:"\u00b0\u0036"
        group : "[Channel1]"
        item: "volume";
        value: proxy.value / 128
    }
    Pushy {
        source:air;id:beat_align1
        prefix:"\u0090\u0002";
        group:"[Channel1]";
        item:"beat_translate_curpos";
    }
    Pushy {
        source:air;id:quantize1
        prefix:"\u0090\u0003"
        group:"[Channel1]"
        item:"quantize"
    }
    Pushy {
        source:air;id:keylock1
        prefix:"\u0090\u0004"
        group:"[Channel1]"
        item:"keylock"
    }

    Pushy     { source:air;id: beat_align2;prefix:"\u0090\u0018";group : "[Channel2]";item: "beat_translate_curpos"; }
    Toggle    { source:air;id: quantize2;prefix:"\u0090\u0019";group : "[Channel2]";item: "quantize"; }
    Toggle    { source:air;id:play2;prefix:"\u0090(";      group : "[Channel2]";item: "play"; }
    Toggle    { source:air;id:cue2;prefix:"\u0090'";       group : "[Channel2]";item: "cue";  }
    BindValue { source:air;id:rate2;prefix:"\u00b0\u0035"; group : "[Channel2]";item: "rate";  }
    BindValue {
        source:air;id:vol2
        prefix:"\u00b0\u003b"
        group : "[Channel2]"
        item: "volume";
        value: proxy.value / 128
    }

    property Controller controller: RtMidiController {
        id: air
        inputIndex: 1
        outputIndex: -1
        Component.onCompleted: { air.open() }
    }
}
