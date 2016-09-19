
import org.mixxx.qml 0.1
import QtQuick 2.6
import QtQuick.Extras 1.4
import QtQuick.Window 2.2
import QtQuick.Controls 1.5

QtObject {
    id: root
    property string group
    property var controls
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
                    var obj = component.createObject(root, {'group' : root.group, 'item': root.controls[i]})
                    console.log(obj)
                    ncontrols[i] = obj;
                }
                root.controls = ncontrols;
            }else if (component.status == Component.Error) {
                console.log("Error loading component: " + component.errorString())
            }
        }
    }
}
