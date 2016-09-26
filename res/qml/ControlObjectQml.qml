import org.mixxx.qml 0.1
import QtQml 2.2

ControlObjectScript {
    id: root
    group: parent.group
    Component.onCompleted: {
        console.log(JSON.stringify(this));
    }
//    onValueChanged : { console.log("value change for " + group + ", " + item + ":\t" + value);}
}
