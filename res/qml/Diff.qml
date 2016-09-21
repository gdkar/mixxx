import org.mixxx.qml 0.1
import QtQuick 2.6
import QtQuick.Extras 1.4
import QtQuick.Window 2.2
import QtQuick.Controls 1.5


ControlProxy {
    id: co
    property string prefix
    property Controller source: parent.controller
    property var proxy: source.getBindingFor(prefix)
    function accumulate(val) { co.fetch_add(val >= 64 ? (val-128) : val); }
    Component.onCompleted: {
        proxy.messageReceived.connect(accumulate)
    }
}
