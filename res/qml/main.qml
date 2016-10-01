import org.mixxx.qml 0.1
import "./keybindings"
import QtQml 2.2
import QtQuick 2.7
Item {

    id: root
//    focus: true
    Loader {
        id: termLoader
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
    Loader {
        id: keyboardLoader
        source: "./Kbd.qml"
//        Kbd { }
    }
    KeyBinder {
        seq: "Ctrl+Alt+Esc"
        Keys.onPressed: {
            console.log("reloading.")
            keyboardLoader.sourceComponent = null
            keyboardLoader.source = "./Kbd.qml";
            termLoader.sourceComponent = null
            event.accepted = true
        }
    }
    KeyBinder {
        seq: "Ctrl+Alt+I"
        Keys.onPressed: {
            termLoader.source = "./Terminal.qml";
            event.accepted = true;
        }
    }
    LadspaLibrary {
        id: ladspaLib
        fileName : "/usr/lib/ladspa/ladspa-rubberband.so"
    }
    Loader {
        id: controllerLoader
        source : "../controllers/HerculesAir.qml"
    }
    Component.onCompleted: {
        ladspaLib.open("/usr/lib/ladspa/ladspa-rubberband.so")
        console.log(ladspaLib)
        for(var i = ladspaLib.plugins.length; i--;) {
            for(var key in ladspaLib.plugins[i]) {
                console.log(key, ladspaLib.plugins[i][key]);
            }
        }
    }
}
