#include <QApplication>
#include <QtDebug>

#include "control/controlproxy.h"
#include "control/control.h"

ControlProxy::ControlProxy(QObject* pParent)
        : QObject(pParent),
          m_pControl(NULL) {
}

ControlProxy::ControlProxy(QString g, QString i, QObject* pParent)
        : QObject(pParent) {
    initialize(ConfigKey(g, i));
}

ControlProxy::ControlProxy(const char* g, const char* i, QObject* pParent)
        : QObject(pParent) {
    initialize(ConfigKey(g, i));
}

ControlProxy::ControlProxy(ConfigKey key, QObject* pParent)
        : QObject(pParent) {
    initialize(key);
}

void ControlProxy::initialize(ConfigKey key) {
    m_key = key;
    // Don't bother looking up the control if key is NULL. Prevents log spew.
    if (!key.isNull()) {
        m_pControl = ControlDoublePrivate::getControl(key);
        if(m_pControl){
            connect(
                m_pControl.data()
              ,&ControlDoublePrivate::trigger
              , this
              ,&ControlProxy::triggered
              , static_cast<Qt::ConnectionType>(
                    Qt::AutoConnection
                  | Qt::UniqueConnection
                    )
                );
            connect(
                m_pControl.data()
              ,&ControlDoublePrivate::defaultValueChanged
              , this
              ,&ControlProxy::defaultValueChanged
              , static_cast<Qt::ConnectionType>(
                    Qt::AutoConnection
                  | Qt::UniqueConnection
                    )
                );
        }
    }
}
void ControlProxy::trigger()
{
    if(m_pControl)
        m_pControl->trigger();
}
ControlProxy::~ControlProxy() {
    //qDebug() << "ControlProxy::~ControlProxy()";
}

bool ControlProxy::connectValueChanged(const QObject* receiver,
        const char* method, Qt::ConnectionType requestedConnectionType)
{

    if (!m_pControl) {
        return false;
    }

    // We connect to the
    // ControlObjectPrivate only once and in a way that
    // the requested ConnectionType is working as desired.
    // We try to avoid direct connections if not requested
    // since you cannot safely delete an object with a pending
    // direct connection. This fixes bug Bug #1406124
    // requested: Auto -> COP = Auto / SCO = Auto
    // requested: Direct -> COP = Direct / SCO = Direct
    // requested: Queued -> COP = Queued / SCO = Auto
    // requested: BlockingQueued -> Assert(false)

    const char* copSlot;
    Qt::ConnectionType copConnection;
    Qt::ConnectionType scoConnection;
    switch(requestedConnectionType) {
    case Qt::AutoConnection:
        copSlot = SLOT(slotValueChangedAuto(double, QObject*));
        copConnection = Qt::AutoConnection;
        scoConnection = Qt::AutoConnection;
        break;
    case Qt::DirectConnection:
        copSlot = SLOT(slotValueChangedDirect(double, QObject*));
        copConnection = Qt::DirectConnection;
        scoConnection = Qt::DirectConnection;
        break;
    case Qt::QueuedConnection:
        copSlot = SLOT(slotValueChangedQueued(double, QObject*));
        copConnection = Qt::QueuedConnection;
        scoConnection = Qt::AutoConnection;
        break;
    case Qt::BlockingQueuedConnection:
        // We must not block the signal source by a blocking connection
        DEBUG_ASSERT(false);
        return false;
    default:
        DEBUG_ASSERT(false);
        return false;
    }

    if (!connect((QObject*)this, SIGNAL(valueChanged(double)),
                      receiver, method, scoConnection)) {
        return false;
    }

    // Connect to ControlObjectPrivate only if required. Do not allow
    // duplicate connections.

    // use only explicit direct connection if requested
    // the caller must not delete this until the all signals are
    // processed to avoid segfaults
    connect(m_pControl.data(), SIGNAL(valueChanged(double, QObject*)),
            this, copSlot,
            static_cast<Qt::ConnectionType>(copConnection | Qt::UniqueConnection));
    return true;
}

// connect to parent object
bool ControlProxy::connectValueChanged(
        const char* method, Qt::ConnectionType type) {
    DEBUG_ASSERT(parent() != NULL);
    return connectValueChanged(parent(), method, type);
}
ConfigKey ControlProxy::getKey() const
{
    return m_key;
}

bool ControlProxy::valid() const { return m_pControl != NULL; }

// Returns the value of the object. Thread safe, non-blocking.
double ControlProxy::get() const {
    return m_pControl ? m_pControl->get() : 0.0;
}

// Returns the bool interpretation of the value
bool ControlProxy::toBool() const {
    return get() > 0.0;
}

// Returns the parameterized value of the object. Thread safe, non-blocking.
double ControlProxy::getParameter() const {
    return m_pControl ? m_pControl->getParameter() : 0.0;
}

// Returns the parameterized value of the object. Thread safe, non-blocking.
double ControlProxy::getParameterForValue(double value) const {
    return m_pControl ? m_pControl->getParameterForValue(value) : 0.0;
}

// Returns the normalized parameter of the object. Thread safe, non-blocking.
double ControlProxy::getDefault() const {
    return m_pControl ? m_pControl->defaultValue() : 0.0;
}

// Set the control to a new value. Non-blocking.
void ControlProxy::slotSet(double v) {
    set(v);
}
// Sets the control value to v. Thread safe, non-blocking.
void ControlProxy::set(double v) {
    if (m_pControl) {
        m_pControl->set(v, this);
    }
}
// Sets the control parameterized value to v. Thread safe, non-blocking.
void ControlProxy::setParameter(double v) {
    if (m_pControl) {
        m_pControl->setParameter(v, this);
    }
}
// Resets the control to its default value. Thread safe, non-blocking.
void ControlProxy::reset() {
    if (m_pControl) {
        // NOTE(rryan): This is important. The originator of this action does
        // not know the resulting value so it makes sense that we should emit a
        // general valueChanged() signal even though the change originated from
        // us. For this reason, we provide NULL here so that the change is
        // not filtered in valueChanged()
        m_pControl->reset();
    }
}
void ControlProxy::slotValueChangedDirect(double v, QObject* pSetter) {
    if (pSetter != this) {
        // This is base implementation of this function without scaling
        emit(valueChanged(v));
    }
}

// Receives the value from the master control by a unique auto connection
void ControlProxy::slotValueChangedAuto(double v, QObject* pSetter) {
    if (pSetter != this) {
        // This is base implementation of this function without scaling
        emit(valueChanged(v));
    }
}

// Receives the value from the master control by a unique Queued connection
void ControlProxy::slotValueChangedQueued(double v, QObject* pSetter) {
    if (pSetter != this) {
        // This is base implementation of this function without scaling
        emit(valueChanged(v));
    }
}

