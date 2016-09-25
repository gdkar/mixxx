import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

Item {
    id: root
    focus:true
    property string seq
    property alias sequence: root.seq
    property alias key: root.seq
    property bool allowAuto: false
    Component.onCompleted: {
        Keyboard.getBindingFor(seq).targets = root;
    }
}
