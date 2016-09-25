import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

KeyBinder {
    id: kb
    property alias group: cp.group
    property alias item:  cp.item
    ControlProxy {
        property string key
        id: cp; group: kb.parent.group }
    Keys.onPressed: {
        if(!event.isAutoRepeat) {
            cp.fetch_toggle();
            event.accepted=true;
        }
    }
}
