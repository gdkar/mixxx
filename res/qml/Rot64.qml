import org.mixxx.qml 0.1
import QtQuick 2.6
import QtQuick.Extras 1.4
import QtQuick.Window 2.2
import QtQuick.Controls 1.5

Item {
    id: root
    property string prefix
    property string group
    property string item
    property bool   invert: False
    property bool   fast:   False
    property Controller source: parent.controller
    property var proxy: source.getBindingFor(prefix)
    ControlProxy {
        id: co
        group: root.group
        item: root.item
        function accumulate(val) {
            var diff = val - 64
            if(fast) {
                co.fetch_add(diff * 1.5);
            }else {
                if( diff == 1 || diff == -1)
                    diff /= 16.
                else
                    diff += (diff>0 ? -1 :+1)
                co.fetch_add(invert ? -diff : diff)
            }
        }
        Component.onCompleted: {
            proxy.messageReceived.connect(accumulate)
        }
    }
}
