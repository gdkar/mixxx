import QtQuick 2.7
import QtQuick.Extras 1.4
import QtQuick.Controls 1.5
import org.mixxx.qml 0.1

Item {
    id: root
    property MidiController source
    property string prefix
    property real   value
    signal messageReceived(int channel, int control, int value, int status, real timestamp)
}
