#include <QApplication>
#include <QtDebug>

#include "controlobjectslave.h"
#include "control/control.h"

ControlObjectSlave::ControlObjectSlave(QObject* pParent)
        : QObject(pParent){
}
ControlObjectSlave::ControlObjectSlave(const QString& g, const QString& i, QObject* pParent)
        : QObject(pParent) {
    initialize(ConfigKey(g, i));
}
ControlObjectSlave::ControlObjectSlave(const char* g, const char* i, QObject* pParent)
        : QObject(pParent) {
    initialize(ConfigKey(g, i));
}
ControlObjectSlave::ControlObjectSlave(const ConfigKey& key, QObject* pParent)
        : QObject(pParent) {
    initialize(key);
}
void ControlObjectSlave::initialize(const ConfigKey& key) {
    m_key = key;
    // Don't bother looking up the control if key is NULL. Prevents log spew.
    if (!key.isNull()) {m_pControl = ControlDoublePrivate::getControl(key);}
}
bool ControlObjectSlave::connectValueChanged(const QObject* receiver,const char* method, Qt::ConnectionType type) {
    auto ret = false;
    if (m_pControl) {
        ret = connect((QObject*)this, SIGNAL(valueChanged(double)),receiver, method, type);
        if (ret) {
            // Connect to ControlObjectPrivate only if required. Do not allow
            // duplicate connections.
            connect(m_pControl.data(), SIGNAL(valueChanged(double, QObject*)),
                    this, SLOT(slotValueChanged(double, QObject*)),
                    static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection));
        }
    }
    return ret;
}
void ControlObjectSlave::emitValueChanged(){valueChanged(get());}
bool ControlObjectSlave::valid()const{return m_pControl;}
double ControlObjectSlave::get()const{return m_pControl?m_pControl->get():0.0;}
double ControlObjectSlave::getDefault()const{return m_pControl?m_pControl->defaultValue():0.0;}
bool ControlObjectSlave::toBool()const{return get()>0;}
double ControlObjectSlave::getParameter()const{return m_pControl?m_pControl->getParameter():0.0;}
double ControlObjectSlave::getParameterForValue(double v)const
{return m_pControl?m_pControl->getParameterForValue(v):0.0;}
const ConfigKey&ControlObjectSlave::getKey()const{return m_key;}
void ControlObjectSlave::set(double v){if ( m_pControl ) m_pControl->set(v,this);}
void ControlObjectSlave::setParameter(double v){if(m_pControl)m_pControl->setParameter(v,this);}
void ControlObjectSlave::reset(){if(m_pControl)m_pControl->reset();}
void ControlObjectSlave::slotValueChanged(double v, QObject *pSetter) { if ( pSetter != this ) emit(valueChanged(v)); }
// connect to parent object
bool ControlObjectSlave::connectValueChanged( const char* method, Qt::ConnectionType type) {
    DEBUG_ASSERT(parent());
    return connectValueChanged(parent(), method, type);
}
