import org.mixxx.qml 0.1
import QtQml 2.2

ControlProxy {
    id: co
    property string prefix
    property bool   invert: False
    property bool   fast:   False
    property Controller source: parent.controller
    property var proxy: source.getBindingFor(prefix)
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
