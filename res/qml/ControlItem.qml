import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

ControlProxy {
    id: co
    property QtObject parent
    group: parent.group
    property string prefix
    property Controller source: parent.controller
    property BindProxy proxy: source.getBindingFor(prefix)
}
