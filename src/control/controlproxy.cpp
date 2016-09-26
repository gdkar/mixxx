#include <QApplication>
#include <QtDebug>
#include "control/controlproxy.h"
#include "control/control.h"

ControlProxy::ControlProxy(QObject* pParent)
        : QObject(pParent),
          m_pControl(nullptr)
{ }

ControlProxy::ControlProxy(QString g, QString i, QObject* pParent)
        : QObject(pParent) {
    initialize(ConfigKey(g, i));
}

ControlProxy::ControlProxy(ConfigKey key, QObject* pParent)
        : QObject(pParent) {
    initialize(key);
}
void ControlProxy::initialize() const
{
    // Don't bother looking up the control if key is NULL. Prevents log spew.
    if (!m_key.isNull()) {
            if(m_pControl){
            auto _valid = false;
            auto _val   = 0.;
            auto _def   = 0.;
            if(m_pControl) {
                _val = m_pControl->get();
                _def = m_pControl->defaultValue();
                _valid = true;
                disconnect(m_pControl.data(),nullptr, this, nullptr);
            }
            if((m_pControl = ControlDoublePrivate::getIfExists(m_key))){
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
                connect(
                    m_pControl.data()
                ,&ControlDoublePrivate::valueChanged
                , this
                ,&ControlProxy::valueChanged
                , static_cast<Qt::ConnectionType>(
                        Qt::AutoConnection
                    | Qt::UniqueConnection
                        )
                    );
                if(valid() != _valid)
                    emit validChanged(valid());
                if(_val != get())
                    emit valueChanged(get());
                if(_def != getDefault())
                    emit defaultValueChanged(getDefault());
            }else{
                if(_valid)
                    emit validChanged(false);
                if(_val)
                    emit valueChanged(0.);
                if(_def)
                    emit defaultValueChanged(0.);
            }
        }
    }
}

void ControlProxy::initialize(ConfigKey key)
{
    m_key = key;
    // Don't bother looking up the control if key is NULL. Prevents log spew.
    if (!key.isNull()) {
        auto _valid = valid();
        auto _val = get();
        auto _def = getDefault();
        if(m_pControl) {
            disconnect(m_pControl.data(),nullptr, this, nullptr);
        }
        m_pControl = ControlDoublePrivate::getIfExists(key);
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
            connect(
                m_pControl.data()
              ,&ControlDoublePrivate::valueChanged
              , this
              ,&ControlProxy::valueChanged
              , static_cast<Qt::ConnectionType>(
                    Qt::AutoConnection
                  | Qt::UniqueConnection
                    )
                );
        }
        if(valid() != _valid)
            emit validChanged(valid());
        if(_val != get())
            emit valueChanged(get());
        if(_def != getDefault())
            emit defaultValueChanged(getDefault());
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
bool ControlProxy::connectValueChanged(const char* method, Qt::ConnectionType type)
{
    DEBUG_ASSERT(parent() != NULL);
    return connectValueChanged(parent(), method, type);
}
ConfigKey ControlProxy::getKey() const
{
    return m_key;
}

bool ControlProxy::valid() const
{
    return m_pControl != NULL;
}
// Returns the value of the object. Thread safe, non-blocking.
double ControlProxy::get() const
{
    return m_pControl ? m_pControl->get() : 0.0;
}
// Returns the bool interpretation of the value
bool ControlProxy::toBool() const
{
    return get() > 0.0;
}
// Returns the parameterized value of the object. Thread safe, non-blocking.
double ControlProxy::getParameter() const
{
    return m_pControl ? m_pControl->getParameter() : 0.0;
}
// Returns the parameterized value of the object. Thread safe, non-blocking.
double ControlProxy::getParameterForValue(double value) const {
    return m_pControl ? m_pControl->getParameterForValue(value) : 0.0;
}
// Returns the normalized parameter of the object. Thread safe, non-blocking.
double ControlProxy::getDefault() const
{
    return m_pControl ? m_pControl->defaultValue() : 0.0;
}
// Sets the control value to v. Thread safe, non-blocking.
void ControlProxy::set(double v)
{
    if (m_pControl) {
        m_pControl->set(v, this);
    }
}
// Sets the control parameterized value to v. Thread safe, non-blocking.
void ControlProxy::setParameter(double v)
{
    if (m_pControl) {
        m_pControl->setParameter(v, this);
    }
}
// Resets the control to its default value. Thread safe, non-blocking.
void ControlProxy::reset()
{
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
void ControlProxy::slotValueChangedAuto(double v, QObject* pSetter)
{
    if (pSetter != this) {
        // This is base implementation of this function without scaling
        emit(valueChanged(v));
    }
}
// Receives the value from the master control by a unique Queued connection
void ControlProxy::slotValueChangedQueued(double v, QObject* pSetter)
{
    if (pSetter != this) {
        // This is base implementation of this function without scaling
        emit(valueChanged(v));
    }
}
ConfigKey ControlProxy::key() const
{
    return m_key;
}
void ControlProxy::setKey(ConfigKey new_key)
{
    if(new_key != key()) {
        auto group_changed = new_key.group() != group();
        auto item_changed  = new_key.item()  != item();
        if(m_pControl)
            QObject::disconnect(m_pControl.data(), 0, this, 0);
        auto _value = get();
        auto _default = getDefault();
        initialize(new_key);

        emit keyChanged(key());
        if(group_changed)
            emit groupChanged(group());
        if(item_changed)
            emit itemChanged(item());
        if(_value != get())
            emit valueChanged(get());
        if(_default != getDefault())
            emit defaultValueChanged(getDefault());
    }
}
void ControlProxy::setGroup(QString _group)
{
    if(_group != group()) {
        setKey({_group,item()});
    }
}
void ControlProxy::setItem(QString _item)
{
    if(_item != item()) {
        setKey({group(),_item});
    }
}
QString ControlProxy::group() const
{
    return m_key.group();
}
QString ControlProxy::item() const
{
    return m_key.item();
}
double ControlProxy::operator += ( double incr)
{
    if(m_pControl) {
        return m_pControl->updateAtomically([incr](double x){return x + incr;});
    }
    return 0;
}
double ControlProxy::operator -= ( double incr)
{
    if(m_pControl) {
        return m_pControl->updateAtomically([incr](double x){return x - incr;});
    }
    return 0;
}
double ControlProxy::operator ++ (int)
{
    if(m_pControl) {
        return m_pControl->updateAtomically([](double x){return x + 1;});
    }
    return 0;
}
double ControlProxy::operator -- (int)
{
    if(m_pControl) {
        return m_pControl->updateAtomically([](double x){return x - 1;});
    }
    return 0;
}
double ControlProxy::operator ++ () { return (++(*this))+1; }
double ControlProxy::operator -- () { return (--(*this))-1; }

double ControlProxy::add_and_saturate(double val, double low_bar, double hi_bar)
{
    if(!m_pControl)
        return 0.0;
    if(!val)
        return get();
    std::tie(low_bar,hi_bar) = std::minmax(low_bar,hi_bar);
    if(val > 0)
        return m_pControl->updateAtomically([val,low_bar,hi_bar](double x)
        {
            return std::min(hi_bar,x + val);
        });
    else
        return m_pControl->updateAtomically([val,low_bar,hi_bar](double x)
        {
            return std::max(low_bar,x + val);
        });

}
double ControlProxy::fetch_add(double val)
{
    return m_pControl ? m_pControl->updateAtomically([val](double x){return x + val;}) : 0.0;
}
double ControlProxy::fetch_sub(double val)
{
    return m_pControl ? m_pControl->updateAtomically([val](double x){return x - val;}) : 0.0;
}
double ControlProxy::compare_exchange(double expected, double desired)
{
    if(m_pControl) {
        m_pControl->compare_exchange_strong(expected,desired);
        return expected;
    }else{
        return 0.0;
    }
}
double ControlProxy::exchange(double val)
{
    return m_pControl ? m_pControl->exchange(val) : 0.0;
}
double ControlProxy::fetch_mul(double val)
{
    return m_pControl ? m_pControl->updateAtomically([val](double x){return val * x;}) : 0.0;
}
double ControlProxy::fetch_div(double val)
{
    return m_pControl ? m_pControl->updateAtomically([val](double x){return val / x;}) : 0.0;
}
double ControlProxy::fetch_toggle()
{
    return m_pControl ? m_pControl->updateAtomically([](double x){return !x;}) : 0.0;
}
