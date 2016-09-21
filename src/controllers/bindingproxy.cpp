#include "controllers/bindingproxy.h"

BindingProxy::BindingProxy(QObject *p)
: QObject(p) {}

BindingProxy::BindingProxy(QString pre,  QObject *p)
: QObject(p)
, m_prefix(pre){}

BindingProxy::~BindingProxy() = default;
double BindingProxy::value() const { return m_value;}
QString BindingProxy::prefix() const { return m_prefix;}
void BindingProxy::setValue(double val)
{
    auto changed = val != m_value;
    m_value = val;
    messageReceived(val);
    if(changed)
        valueChanged(val);
}
void BindingProxy::setPrefix(QString pre)
{
    if(pre != m_prefix)
        prefixChanged(m_prefix = pre);
}
