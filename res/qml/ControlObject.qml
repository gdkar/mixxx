import org.mixxx.qml 0.1

ControlObjectScript {
    id: root
    group: parent.group
    onValueChanged : { console.log("value change for " + group + ", " + item + ":\t" + value);}
}
