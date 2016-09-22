#include "controllers/bindingproxy.h"

BindingProxy::BindingProxy(QObject *p)
: BindingProxy(QVariant{},p){
}

BindingProxy::BindingProxy(QVariant pre,  QObject *p)
: QObject(p)
, m_prefix(pre){
    connect(this, &BindingProxy::valueChanged,
        this, &BindingProxy::messageReceived,
        Qt::DirectConnection);
}

BindingProxy::~BindingProxy() = default;
QVariant BindingProxy::value() const { return m_value;}
QVariant BindingProxy::prefix() const { return m_prefix;}
void BindingProxy::setValue(QVariant val)
{
    auto changed = val != m_value;
    m_value = val;
    if(changed)
        valueChanged(val);
    else
        messageReceived(val);
}
void BindingProxy::setPrefix(QVariant pre)
{
    if(pre != m_prefix)
        prefixChanged(m_prefix = pre);
}
