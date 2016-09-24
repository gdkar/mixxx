import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

Item {
    id: root
    focus:true
    property string key
    property alias sequence: root.key
    property bool allowAuto: false
    Component.onCompleted: {
        Keyboard.getBindingFor(key).targets = root;
    }
}
