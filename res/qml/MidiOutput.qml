import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

ControlObjectScript {
    id: root
    group: parent.group
    property QtObject parent
    property var midi: parent.controller
    property int midiStatus: 0x90
    property int midiNumber
    property int midiValueOn: 0x00
    property int midiValueOff:0x7f
    property real minimum: 0.1
    property real maximum: 1.0
    property bool previous: false
    function setOutput(val) {
        var current = (val >= this.minimum && val <= this.maximum);
        if(current != this.previous) {
            midi.sendShortMsg(this.midiStatus,this.midiNumber,
                current ? this.midiValueOn : this.midiValueOff);
            this.previous = this.current
        }
    }
    onValueChanged: root.setOutput
}
