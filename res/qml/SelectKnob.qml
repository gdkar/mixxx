import org.mixxx.qml 0.1
import QtQml 2.6

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
