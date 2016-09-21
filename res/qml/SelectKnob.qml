import org.mixxx.qml 0.1
import QtQuick 2.6
import QtQuick.Extras 1.4
import QtQuick.Window 2.2
import QtQuick.Controls 1.5

ControlProxy {
    id: co
    property Controller source: parent.controller
    property string prefix
    property var proxy: source.getBindingFor(prefix)
    function accumulate(val) {
        co.set(val >= 64 ? (val-128) : val);
    }
    Component.onCompleted: {
        proxy.messageReceived.connect(accumulate)
    }
}
