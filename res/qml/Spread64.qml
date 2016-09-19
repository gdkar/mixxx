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
    property Controller source: parent.controller
    property var proxy: source.getBindingFor(prefix)
    ControlProxy {
        id: co
        group: root.group
        item: root.item
        function accumulate(val) { co.set(val -64. ); }
        Component.onCompleted: {
            proxy.messageReceived.connect(accumulate)
        }
    }
}
