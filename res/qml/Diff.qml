import org.mixxx.qml 0.1
import QtQml 2.2

ControlItem {
    id: co
    Connections {
        target: co.proxy;
        onMessageReceived: { co.fetch_add(val >= 64 ? (val-128) : val);}
    }
}
