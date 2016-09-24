import org.mixxx.qml 0.1
import QtQml 2.2

ControlItem {
    id: co
    function toggle (val){ if(val==127) co.fetch_toggle()}
    Component.onCompleted: {co.proxy.valueChanged.connect(co.toggle)}
}
