import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

ControlObject {
    id: root
    group: parent.group
    item:  "beat_active"
    property QtObject parent
    property var midi: parent.controller
    property int stepStart: 0x44
    property int stepStop: 0x47
    property int step1: 0
    property int step2: stepStart
    function advance(val) {
        if(val == 1.) {
            if(root.step1 != 0)
                midi.sendShortMsg(0x90, root.step1, 0x00);
            root.step1 = root.step2
            midi.sendShortMsg(0x90, root.step2, 0x7f);
            if(root.step2 < root.stepStop)
                root.step2++;
            else
                root.step2 = root.stepStart;
        }
    }
    function restart(val) { if(val == 0) {root.step1 = 0;root.step2 = root.stepStart;}}
    property var playproxy : ControlProxy {
        id:play;
        group:root.group;
        item: "play"
        onValueChanged: { root.restart(value) }
    }
    onValueChanged: { root.advance(value) }
}
