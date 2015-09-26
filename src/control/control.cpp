#include <QtDebug>
#include <QMutexLocker>

#include "control/control.h"
#include "controlobject.h"
#include "util/stat.h"
#include "util/timer.h"

// Static member variable definition
ConfigObject<ConfigValue>* ControlDoublePrivate::s_pUserConfig = nullptr;
QHash<ConfigKey, QSharedPointer<ControlDoublePrivate> > ControlDoublePrivate::s_qCOHash;
QHash<ConfigKey, ConfigKey> ControlDoublePrivate::s_qCOAliasHash;
QMutex ControlDoublePrivate::s_qCOHashMutex;

/*
ControlDoublePrivate::ControlDoublePrivate()
        : m_bIgnoreNops(true),
          m_bTrack(false),
          m_trackType(Stat::UNSPECIFIED),
          m_trackFlags(Stat::COUNT | Stat::SUM | Stat::AVERAGE |
                       Stat::SAMPLE_VARIANCE | Stat::MIN | Stat::MAX),
          m_confirmRequired(false) {
    initialize();
}
*/

ControlDoublePrivate::ControlDoublePrivate(ConfigKey key,
                                           ControlObject* pCreatorCO,
                                           bool bIgnoreNops, bool bTrack,
                                           bool bPersist)
        : m_key(key),
          m_bPersistInConfiguration(bPersist),
          m_bIgnoreNops(bIgnoreNops),
          m_bTrack(bTrack),
          m_trackKey("control " + m_key.group + "," + m_key.item),
          m_trackType(Stat::UNSPECIFIED),
          m_trackFlags(Stat::COUNT | Stat::SUM | Stat::AVERAGE | Stat::SAMPLE_VARIANCE | Stat::MIN | Stat::MAX),
          m_confirmRequired(false),
          m_pCreatorCO(pCreatorCO) {
    initialize();
}
void ControlDoublePrivate::initialize() {
    auto value = 0.0;
    if (m_bPersistInConfiguration) {
        if(auto pConfig = ControlDoublePrivate::s_pUserConfig){
            // Assume toDouble() returns 0 if conversion fails.
            value = pConfig->getValueString(m_key).toDouble();
        }
    }
    m_defaultValue.store(0);
    m_value.store(value);
    //qDebug() << "Creating:" << m_trackKey << "at" << &m_value << sizeof(m_value);
    if (m_bTrack) {
        // TODO(rryan): Make configurable.
        Stat::track(m_trackKey, static_cast<Stat::StatType>(m_trackType),static_cast<Stat::ComputeFlags>(m_trackFlags),m_value.load());
    }
}
/* static */ void
ControlDoublePrivate::setUserConfig(ConfigObject<ConfigValue>* pConfig){s_pUserConfig = pConfig;}
ControlDoublePrivate::~ControlDoublePrivate() {
    {
      QMutexLocker locker(&s_qCOHashMutex);
      {
        auto pControl = s_qCOHash.value(m_key);
        if ( pControl == this )
        {
          s_qCOHash.remove(m_key);
        }
      }
    }
    if (m_bPersistInConfiguration)
        if(auto pConfig = ControlDoublePrivate::s_pUserConfig) pConfig->set(m_key, QString::number(get()));
    if(m_pCreatorCO) delete m_pCreatorCO;
}
// static
void ControlDoublePrivate::insertAlias(const ConfigKey& alias, const ConfigKey& key) {
    QMutexLocker locker(&s_qCOHashMutex);
    QSharedPointer<ControlDoublePrivate> pControl;
    if(s_qCOHash.contains(key))
    {
      pControl = s_qCOHash.value(key);
    }
    if (pControl.isNull()) {
        qWarning() << "WARNING: ControlDoublePrivate::insertAlias called for expired control" << key;
        return;
    }
    s_qCOAliasHash.insert(key, alias);
    s_qCOHash.insert(alias, pControl);
}
// static
QSharedPointer<ControlDoublePrivate> ControlDoublePrivate::getControl(const ConfigKey& key, bool warn, ControlObject* pCreatorCO,bool bIgnoreNops, bool bTrack, bool bPersist) {
    if (key.isNull()) {
        if (warn)
            qWarning() << "ControlDoublePrivate::getControl returning NULL" << "for empty ConfigKey.";
        return QSharedPointer<ControlDoublePrivate>();
    }
    QMutexLocker locker(&s_qCOHashMutex);
    auto pControl = QSharedPointer<ControlDoublePrivate>{};
    if(s_qCOHash.contains(key))  pControl = s_qCOHash.value(key);
    if (!pControl) {
            pControl = QSharedPointer<ControlDoublePrivate>(new ControlDoublePrivate(
                  key, 
                  pCreatorCO, 
                  bIgnoreNops,
                  bTrack, 
                  bPersist
                  )
                );
            //qDebug() << "ControlDoublePrivate::s_qCOHash.insert(" << key.group << "," << key.item << ")";
            s_qCOHash.insert(key, pControl);
            locker.unlock();
        if (!pCreatorCO) pControl->m_pCreatorCO = new ControlObject(key,bIgnoreNops,bTrack,bPersist);
    }
    return pControl;
}
QString ControlDoublePrivate::name() const {return m_name;}
void ControlDoublePrivate::setName(const QString &s){m_name = s;}
QString ControlDoublePrivate::description() const { return m_description;}
void ControlDoublePrivate::setDescription(const QString &s){m_description = s;}
double ControlDoublePrivate::get()const{return m_value.load();}
bool ControlDoublePrivate::ignoreNops() const{return m_bIgnoreNops;}
void ControlDoublePrivate::setDefaultValue(double dValue){m_defaultValue.store(dValue);}
double ControlDoublePrivate::defaultValue()const{return m_defaultValue.load();}
ControlObject *ControlDoublePrivate::getCreatorCO() const{ return m_pCreatorCO;}
void ControlDoublePrivate::removeCreatorCO(){m_pCreatorCO = nullptr;}
ConfigKey ControlDoublePrivate::getKey() const{return m_key;}
// static
void ControlDoublePrivate::getControls( QList<QSharedPointer<ControlDoublePrivate> >* pControlList) {
    s_qCOHashMutex.lock();
    pControlList->clear();
    for ( auto it = s_qCOHash.cbegin(),end=s_qCOHash.cend();it!=end;++it){
      if(auto pControl = it.value() ) pControlList->push_back(pControl);
    }
    s_qCOHashMutex.unlock();
}
// static
QHash<ConfigKey, ConfigKey> ControlDoublePrivate::getControlAliases() {
    QMutexLocker locker(&s_qCOHashMutex);
    return s_qCOAliasHash;
}
void ControlDoublePrivate::reset() {
    // NOTE: pSender = NULL is important. The originator of this action does
    // not know the resulting value so it makes sense that we should emit a
    // general valueChanged() signal even though we know the originator.
    set(m_defaultValue.load(), nullptr);
}
void ControlDoublePrivate::set(double value, QObject* pSender) {
    // If the behavior says to ignore the set, ignore it.
    auto pBehavior = m_pBehavior;
    if (!pBehavior.isNull() && !pBehavior->setFilter(&value)) {return;}
    if (m_confirmRequired) { emit(valueChangeRequest(value));}
    else { setInner(value, pSender);}
}
void ControlDoublePrivate::setAndConfirm(double value, QObject* pSender) { setInner(value, pSender); }
void ControlDoublePrivate::setInner(double value, QObject* pSender) {
    if (m_value.exchange(value) != value)
    {
      emit(valueChanged(value, pSender));
      if (m_bTrack) {Stat::track(m_trackKey, static_cast<Stat::StatType>(m_trackType), static_cast<Stat::ComputeFlags>(m_trackFlags), value);}
    }
}
void ControlDoublePrivate::setBehavior(ControlNumericBehavior* pBehavior) {
    // This marks the old mpBehavior for deletion. It is deleted once it is not
    // used in any other function
    m_pBehavior = QSharedPointer<ControlNumericBehavior>(pBehavior);
}
void ControlDoublePrivate::setParameter(double dParam, QObject* pSender) {
    auto pBehavior = m_pBehavior;
    if (pBehavior.isNull()) { set(dParam, pSender);}
    else { pBehavior->setValueFromParameter(dParam,this);}
}
double ControlDoublePrivate::getParameter() const {return getParameterForValue(get()); }
double ControlDoublePrivate::getParameterForValue(double value) const {
    auto pBehavior = m_pBehavior;
    if (!pBehavior.isNull()) {value = pBehavior->valueToParameter(value);}
    return value;
}
bool ControlDoublePrivate::connectValueChangeRequest(const QObject* receiver, const char* method, Qt::ConnectionType type) {
    // confirmation is only required if connect was successful
    m_confirmRequired = connect(this, SIGNAL(valueChangeRequest(double)), receiver, method, type);
    return m_confirmRequired;
}
