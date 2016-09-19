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
    Toggle    { id: play1;prefix:"\u0090\u0012";group : "[Channel1]";item: "play"; }
    Toggle    { id: cue1;prefix:"\u0090\u0011"; group : "[Channel1]";item: "cue";  }
    BindValue { id:rate1;prefix:"\u00b0\u0034"; group : "[Channel1]";item: "rate";  }

    Toggle    { id:play2;prefix:"\u0090(";      group : "[Channel2]";item: "play"; }
    Toggle    { id:cue2;prefix:"\u0090'";       group : "[Channel2]";item: "cue";  }
    BindValue { id:rate2;prefix:"\u00b0\u0035"; group : "[Channel2]";item: "rate";  }

    property var controller: RtMidiController {
        id: air
        inputIndex: 1
        outputIndex: -1
        Component.onCompleted: {
            air.open()
        }
    }
}
