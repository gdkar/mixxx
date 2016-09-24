import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

KeyBinder {
    id: kb
    property real amount: 1.1
    property alias group: cp.group
    property alias item:  cp.item
    ControlProxy { id: cp }
    Keys.onPressed: {
        if(!event.isAutoRepeat || kb.allowAuto){
            cp.fetch_multiply(amount);
            event.accepted=true
        }
    }

}
