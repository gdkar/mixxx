import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

KeyBinder {
    id: kb
    allowAuto:true
    property real amount: 1.
    property real min: -Infinity
    property real max: -Infinity
    property alias group: cp.group
    property alias item:  cp.item
    ControlObjectScript { id: cp;group:kb.parent.group }
    Keys.onPressed: {
        if(allowAuto || !event.isAutoRepeat) {
            cp.modify(function(val) {
                return Math.min(Math.max(kb.min,val+kb.amount),kb.max)
            });
            event.accepted=True
        }
    }
}
