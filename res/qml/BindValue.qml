import org.mixxx.qml 0.1
import QtQml 2.2

ControlProxy {
    id: co
    property string prefix
    property Controller source: parent.controller
    property var proxy: source.getBindingFor(prefix)

    value: (proxy.value-64)/64.
}
