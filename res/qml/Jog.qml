import org.mixxx.qml 0.1
import QtQml 2.2

ControlItem {
    id: co
    property string prefix
    property Controller source: parent.controller
    property var proxy: source.getBindingFor(prefix)
    property real wheel_factor: 0.8
    function jog(val) {
        var newval = (val > 64 ? val - 128 : val)
        co.set(wheel_factor * newval)
    }
    Component.onCompleted: {
        co.proxy.messageReceived.connect(co.jog)
    }
}
