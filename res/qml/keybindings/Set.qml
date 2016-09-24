import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

KeyBinder {
    id: kb
    property alias group: cp.group
    property alias item:  cp.item
    property real to: cp.defaultValue
    ControlProxy {
        id: cp;
        group:kb.parent.group
    }
    Keys.onPressed: {
        if(kb.allowAuto || !event.isAutoRepeat) {
            cp.value = kb.to
            event.accepted = true;
        }
    }
}
