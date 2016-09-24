#include "controllers/bindproxy.h"

BindProxy::BindProxy(QObject *p)
: QObject(p) {}

BindProxy::BindProxy(QString pre,  QObject *p)
: QObject(p)
, m_prefix(pre){}

BindProxy::~BindProxy() = default;
double BindProxy::value() const
{
    return m_value;
}
QString BindProxy::prefix() const
{
    return m_prefix;
}
void BindProxy::setValue(double val)
{
    auto changed = val != m_value;
    m_value = val;
    if(changed) {
      valueChanged(val);
    }
    messageReceived(val);
}
void BindProxy::setPrefix(QString pre)
{
    if(pre != m_prefix) {
        m_prefix.swap(pre);
        prefixChanged(m_prefix = pre);
    }
}
