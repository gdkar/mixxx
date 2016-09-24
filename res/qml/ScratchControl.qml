import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

QtObject {
    id: sc
    property real m_dx: 0
    property real m_intervalAccumulator: 0.
    property bool m_ramp: false
    property real m_rampFactor: 0.0001
    property bool m_brakeActive: false
}
