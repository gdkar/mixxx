import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

KeyBinder {
    id: kb
    property real amount: 1.
    property alias group: cp.group
    property alias item:  cp.item
    ControlProxy {
        id:cp
        group:kb.parent.group
    }
    Keys.onPressed: {
        if(!event.isAutoRepeat) {
            cp.fetch_add(amount);
            event.accepted=true;
        }
    }
    Keys.onReleased: {
        if(!event.isAutoRepeat) {
            cp.fetch_add(-amount);
            event.accepted=true;
        }
    }
}
