import org.mixxx.qml 0.1
import "./keybindings"
import QtQml 2.2
import QtQuick 2.7
Item {

    id: root
    focus: true
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
    Loader {
        id: controllerLoader
        source : "../controllers/HerculesAir.qml"
    }
}
