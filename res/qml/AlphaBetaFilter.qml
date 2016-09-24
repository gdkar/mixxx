import org.mixxx.qml 0.1
import QtQml 2.2
import QtQuick 2.7

QtObject {
    id: root
    property real m_dt
    property real m_x: 0
    property real m_v
    property real m_alpha: 1./512
    property real m_beta: (1./512)/1024
    property alias predictedVelocity: root.m_v
    property alias predictedPosition: root.m_x
    function observation(dx) {
        var predicted_x = m_x + m_v * m_dt;
        var predicted_v = m_v;
        var residual_x  = dx - predicted_x

        m_x = predicted_x + residual_x * m_alpha
        m_v = predicted_v + residual_v * m_beta / m_dt

        m_x -= dx;
    }
}
