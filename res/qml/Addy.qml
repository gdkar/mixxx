import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

ControlItem {
    id: co
    property real amount: 1.
    property real min: -Infinity
    property real max: +Infinity
    function accumulate(val) {
        if(val == 127)
            co.modify(function(x) {
                var tgt = x + co.amount;
                tgt = Math.min(tgt, co.max);
                tgt = Math.max(tgt, co.min);
                return tgt;
            });
    }
    Component.onCompleted: {
        co.proxy.valueChanged.connect(co.accumulate);
    }
}
