
import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7
Item {
    id: root
    property string group
    property var controls
    property var bindings
    Component.onCompleted: {
        var component = Qt.createComponent("ControlObject.qml")
        if(component.status == Component.Ready) {
            finishCreation()
        }else{
            component.statusChanged.connect(finishCreation)
        }
        function finishCreation() {
            if(component.status == Component.Ready) {
                var ncontrols = new Array(controls.length)
                for(var i = 0; i < controls.length; ++i) {
                    ncontrols[i] = component.createObject(root, {'group' : root.group, 'item': root.controls[i]})
                }
                root.controls = ncontrols;
            }else if (component.status == Component.Error) {
                console.log("Error loading component: " + component.errorString())
            }
        }
    }
}
