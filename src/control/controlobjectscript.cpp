#include <QApplication>
#include <QtDebug>

#include "control/controlobjectscript.h"

ControlObjectScript::ControlObjectScript(const ConfigKey& key, QObject* pParent)
        : ControlProxy(key, pParent) {
}

void ControlObjectScript::connectScriptFunction(
        const ControllerEngineConnection& conn) {
    if (m_connectedScriptFunctions.isEmpty()) {
        // we connect the slots only, if there will be actually a script
        // connected
        connect(m_pControl.data(), &ControlDoublePrivate::valueChanged,
            this, &ControlObjectScript::onValueChanged,
            static_cast<Qt::ConnectionType>(
                Qt::QueuedConnection
              | Qt::UniqueConnection
                )
            );
    }
    m_connectedScriptFunctions.append(conn);
}

bool ControlObjectScript::disconnectScriptFunction(
        const ControllerEngineConnection& conn)
{
    auto ret = m_connectedScriptFunctions.removeAll(conn) > 0;
    if (m_connectedScriptFunctions.isEmpty()) {
        // no script left, we can disconnected
        disconnect(
            m_pControl.data(), &ControlDoublePrivate::valueChanged,
            this, &ControlObjectScript::onValueChanged
            );
    }
    return ret;
}

void ControlObjectScript::onValueChanged(double value)
{
    // Make a local copy of m_connectedScriptFunctions fist.
    // This allows a script to disconnect a callback from the callback
    // itself. Otherwise the this may crash since the disconnect call
    // happens during conn.function.call() in the middle of the loop below.
    auto connections = m_connectedScriptFunctions;
    for(auto&& conn: connections) {
        auto args = QJSValueList{};
        args << value;
        args << getKey().group;
        args << getKey().item;
        auto result = conn.function.callWithInstance(conn.context, args);
        if (result.isError()) {
            qWarning() << "ControllerEngine: Invocation of callback" << conn.id
                       << "failed:" << result.toString();
        }
    }
}
