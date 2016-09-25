import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

KeyBinder {
    id: kb
    property alias group: cp.group
    property alias item:  cp.item
    ControlProxy {
        id: cp;
        property string key:""
        group:kb.parent.group
    }
    Keys.onPressed: {
        if(kb.allowAuto || !event.isAutoRepeat) {
            cp.value = 1;
            event.accepted = true;
        }
    }
    Keys.onReleased:{
        if(kb.allowAuto || !event.isAutoRepeat) {
            cp.value = 0;
            event.accepted = true;
        }
    }
}
