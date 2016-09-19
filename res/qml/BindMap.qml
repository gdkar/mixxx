import QtQuick 2.7
import QtQuick.Extras 1.4
import QtQuick.Controls 1.5
import org.mixxx.qml 0.1

BindProxy {
    id: root
    property var proxy: source.dispatch[prefix]
    Component.onCompleted: {
        if(proxy === undefined) {
            var component = Qt.createComponent("BindProxy.qml")
            if(component.status==Component.Ready){
                finish();
            }else{
                compoent.statusChanged.connect(finish);
            }
            function finish(){
                if(component.status == Component.Ready){
                    proxy = source.dispatch[prefix] = component.createObject(
                        source, { 'prefix': root.prefix });
                    proxy.messageReceived.connect(messageReceived)
                }else if(component.status == Component.Error) {
                    console.log("Error loading component: " + component.errorString());
                }
            }
        }
    }
}
