#include <QApplication>
#include <QtDebug>

#include <QtQml>
#include <QJSValue>

#include "controllers/controllerengine.h"
#include "control/controlobjectscript.h"

ControlObjectScript::ControlObjectScript(const ConfigKey& key, QObject* pParent)
        : ControlProxy(key, pParent) {
}
ControlObjectScript::ControlObjectScript(QString group,QString item, QObject *p)
: ControlObjectScript(ConfigKey(group,item),p)
{ }
ControlObjectScript::ControlObjectScript(QObject *p)
: ControlProxy(p) {}

double ControlObjectScript::modify(QJSValue func, QJSValue _context)
{
    if(!_context.isObject()){
        return modify(func);
    }
    if(func.isCallable() && m_pControl) {
        return m_pControl->updateAtomically([&func,_context](double x) mutable {
            return func.callWithInstance(_context,(QJSValueList{}<<x)).toNumber();
        });
    }
    return 0;
}
double ControlObjectScript::modify(QJSValue func)
{
    if(func.isCallable() && m_pControl) {
        return m_pControl->updateAtomically([&func](double x) mutable {
            return func.call((QJSValueList{}<<x)).toNumber();
        });
    }
    return 0;
}
void ControlObjectScript::connectControl(QJSValue _callback, QJSValue _context)
{
    if(!_callback.strictlyEquals(callback())
    || !_context .strictlyEquals(context())) {
        auto callback_changed = !m_callback.strictlyEquals(_callback);
        auto context_changed = !m_context.strictlyEquals(_context);
        m_callback = _callback;
        m_context  = _context;

        constexpr const auto conn_type = static_cast<Qt::ConnectionType>(
            Qt::QueuedConnection |Qt::UniqueConnection);

        connect(m_pControl.data(), &ControlDoublePrivate::valueChanged,
                this, &ControlProxy::valueChanged,conn_type);
        connect(this, &ControlObjectScript::valueChanged,
                this, &ControlObjectScript::onValueChanged,
                static_cast<Qt::ConnectionType>(
                    Qt::DirectConnection | Qt::UniqueConnection)
                );
        if(callback_changed)
            emit callbackChanged(callback());
        if(context_changed)
            emit contextChanged(context());
    }
}
void ControlObjectScript::disconnectControl()
{
    connectControl(QJSValue{},QJSValue{});
}
QJSValue ControlObjectScript::context() const
{
    return m_context;
}
QJSValue ControlObjectScript::callback() const
{
    return m_callback;
}
void ControlObjectScript::onValueChanged(double value)
{
    // Make a local copy of m_connectedScriptFunctions fist.
    // This allows a script to disconnect a callback from the callback
    // itself. Otherwise the this may crash since the disconnect call
    // happens during conn.function.call() in the middle of the loop below.
    if(m_callback.isCallable()) {
        auto result = m_context.isObject() ? m_callback.callWithInstance(m_context,(QJSValueList{}<<value))
                                           : m_callback.call((QJSValueList{}<<value));
        if (result.isError()) {
            qWarning() << "ControlObjectScript : Invocation of callback"
                       << callback().toString()
                       << "failed:"
                       << result.toString()
                       << "(backtrace: (\n" + result.property("stack").toString() + "\n)";
        }

    }
}
void ControlObjectScript::setCallback(QJSValue _cb)
{
    connectControl(_cb, context());
}
void ControlObjectScript::setContext(QJSValue _ctx)
{
    connectControl(callback(),_ctx);
}
