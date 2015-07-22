#include <QApplication>
#include <QtDebug>

#include "controlobjectslave.h"
#include "control/control.h"

ControlObjectSlave::ControlObjectSlave(QObject* pParent)
        : QObject(pParent),
          m_pControl(nullptr) {
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
    // Don't bother looking up the control if key is nullptr. Prevents log spew.
    if (!key.isNull()) {
        m_pControl = ControlDoublePrivate::getControl(key);
        if(m_pControl)
          connect(m_pControl.data(),            &ControlDoublePrivate::valueChanged,
                  this, &ControlObjectSlave::valueChanged,
                  static_cast<Qt::ConnectionType>(Qt::DirectConnection|Qt::UniqueConnection));
    }
}
ControlObjectSlave::~ControlObjectSlave() {}
bool ControlObjectSlave::connectValueChanged(const QObject* receiver,
        const char* method, Qt::ConnectionType type) {
    bool ret = false;
    if (m_pControl) {
        ret = connect((QObject*)this, SIGNAL(valueChanged(double)),receiver, method, type);
    }
    return ret;
}
// connect to parent object
bool ControlObjectSlave::connectValueChanged(const char* method, Qt::ConnectionType type) {
    DEBUG_ASSERT(parent() != nullptr);
    return connectValueChanged(parent(), method, type);
}
