#include <QtDebug>
#include <QSharedPointer>
#include <initializer_list>
#include "control/control.h"
#include "control/controlobject.h"

#include "util/stat.h"
#include "util/timer.h"

// Static member variable definition
UserSettingsPointer ControlDoublePrivate::s_pUserConfig;

QHash<ConfigKey, QWeakPointer<ControlDoublePrivate> > ControlDoublePrivate::s_qCOHash
GUARDED_BY(ControlDoublePrivate::s_qCOHashMutex);

QHash<ConfigKey, ConfigKey> ControlDoublePrivate::s_qCOAliasHash
GUARDED_BY(ControlDoublePrivate::s_qCOHashMutex);

MMutex ControlDoublePrivate::s_qCOHashMutex;

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
                                           bool bIgnoreNops,
                                           bool bTrack,
                                           bool bPersist)
        : QObject(nullptr),
          m_key(key),
          m_bPersistInConfiguration(bPersist),
          m_bIgnoreNops(bIgnoreNops),
          m_bTrack(bTrack),
          m_trackType(Stat::UNSPECIFIED),
          m_trackFlags(Stat::COUNT | Stat::SUM | Stat::AVERAGE |
                       Stat::SAMPLE_VARIANCE | Stat::MIN | Stat::MAX),
          m_confirmRequired(false),
          m_pCreatorCO(nullptr)
{
    initialize();
}

void ControlDoublePrivate::initialize()
{
    auto value = 0.;
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
        m_trackKey = "control " + m_key.group + "," + m_key.item;
        Stat::track(m_trackKey, static_cast<Stat::StatType>(m_trackType),
                    static_cast<Stat::ComputeFlags>(m_trackFlags),
                    m_value.load());
    }
}

ControlDoublePrivate::~ControlDoublePrivate()
{
    {
        MMutexLocker locker(&s_qCOHashMutex);
        auto it = s_qCOHash.constFind(m_key);
        if(it != s_qCOHash.constEnd()) {
            auto pControl = it.value().lock();
            if(pControl == this || !pControl) {
                s_qCOHash.erase(it);
            }
        }
    }
    if (m_bPersistInConfiguration) {
        if(auto pConfig = ControlDoublePrivate::s_pUserConfig)
            pConfig->set(m_key, QString::number(get()));
    }
}

// static
void ControlDoublePrivate::insertAlias(ConfigKey alias, ConfigKey key)
{
    MMutexLocker locker(&s_qCOHashMutex);

    auto it = s_qCOHash.constFind(key);
    if (it == s_qCOHash.constEnd()) {
        qWarning() << "WARNING: ControlDoublePrivate::insertAlias called for null control" << key;
        return;
    }

    auto pControl = it.value().lock();
    if (pControl.isNull()) {
        qWarning() << "WARNING: ControlDoublePrivate::insertAlias called for expired control" << key;
        return;
    }

    s_qCOAliasHash.insert(key, alias);
    s_qCOHash.insert(alias, pControl);
}
// static
QSharedPointer<ControlDoublePrivate> ControlDoublePrivate::getIfExists(ConfigKey key)
{
    // Scope for MMutexLocker.
    if(!key.isEmpty()){
        MMutexLocker locker(&s_qCOHashMutex);
        auto it = s_qCOHash.find(key);
        if (it != s_qCOHash.end()) {
            return it.value().lock();
//            }
        }
    }
    return QSharedPointer<ControlDoublePrivate>{};
}
// static
QSharedPointer<ControlDoublePrivate> ControlDoublePrivate::getControl(
        ConfigKey key, bool warn, bool bIgnoreNops, bool bTrack, bool bPersist)
{
    if (key.isEmpty()) {
        if (warn) {
            qWarning() << "ControlDoublePrivate::getControl returning NULL"
                       << "for empty ConfigKey.";
        }
        return QSharedPointer<ControlDoublePrivate>();
    }
    QSharedPointer<ControlDoublePrivate> pControl;
    // Scope for MMutexLocker.
    {
        MMutexLocker locker(&s_qCOHashMutex);
        auto it = s_qCOHash.find(key);
        if (it != s_qCOHash.end()) {
            if (warn) {
                qDebug() << "ControlObject" << key.group << key.item << "already created";
            }
            pControl = it.value().lock();
//            }
        }
    }
    if (pControl == NULL) {
        pControl = QSharedPointer<ControlDoublePrivate>::create(
            key
            , bIgnoreNops
            , bTrack
            , bPersist
            );
        {
        pControl->m_pCreatorCO = new ControlObject(pControl.data());
            MMutexLocker locker(&s_qCOHashMutex);
            {
                auto it = s_qCOHash.constFind(key);
                if(it != s_qCOHash.constEnd()) {
                    if(auto oControl = it.value().lock()) {
                        locker.unlock();
                        return oControl;
                    }else{
                        s_qCOHash.erase(it);
                    }
                }
            }
            //qDebug() << "ControlDoublePrivate::s_qCOHash.insert(" << key.group << "," << key.item << ")";
            s_qCOHash.insert(key, pControl);
        }
//        } else if (warn) {
//            qWarning() << "ControlDoublePrivate::getControl returning NULL for ("
//                       << key.group << "," << key.item << ")";
//        }
    }
    return pControl;
}

// static
void ControlDoublePrivate::getControls(
        QList<QSharedPointer<ControlDoublePrivate> >* pControlList)
{
    auto wControlList = QList<QWeakPointer<ControlDoublePrivate> >{};
    {
        MMutexLocker locker(&s_qCOHashMutex);
        wControlList = s_qCOHash.values();
    }
    pControlList->clear();
    for (auto wControl : wControlList) {
        if(auto pControl = wControl.lock())
            pControlList->push_back(pControl);
    }
}

// static
QHash<ConfigKey, ConfigKey> ControlDoublePrivate::getControlAliases()
{
    MMutexLocker locker(&s_qCOHashMutex);
    return s_qCOAliasHash;
}

void ControlDoublePrivate::reset()
{
    // NOTE: pSender = NULL is important. The originator of this action does
    // not know the resulting value so it makes sense that we should emit a
    // general valueChanged() signal even though we know the originator.
    set(defaultValue(), nullptr);
}

void ControlDoublePrivate::set(double value, QObject* pSender)
{
    // If the behavior says to ignore the set, ignore it.
    if(auto pBehavior = m_pBehavior) {
        if(!pBehavior->setFilter(&value))
            return;
    }
    if (m_confirmRequired) {
        emit(valueChangeRequest(value));
    } else {
        setAndConfirm(value, pSender);
    }
}

void ControlDoublePrivate::setAndConfirm(double value, QObject* pSender)
{
    if ( m_value.exchange(value) == value && ignoreNops())
        return;

    valueChanged(value, pSender);

    if (m_bTrack) {
        Stat::track(m_trackKey, static_cast<Stat::StatType>(m_trackType),
                    static_cast<Stat::ComputeFlags>(m_trackFlags), value);
    }
}

void ControlDoublePrivate::setBehavior(ControlNumericBehavior* pBehavior)
{
    // This marks the old mpBehavior for deletion. It is deleted once it is not
    // used in any other function
    m_pBehavior = QSharedPointer<ControlNumericBehavior>(pBehavior);
}

void ControlDoublePrivate::setParameter(double dParam, QObject* pSender)
{
    if(auto pBehavior = m_pBehavior) {
        set(pBehavior->parameterToValue(dParam), pSender);
    }else{
        set(dParam, pSender);
    }
}

double ControlDoublePrivate::getParameter() const
{
    return getParameterForValue(get());
}

double ControlDoublePrivate::getParameterForValue(double value) const
{
    if(auto pBehavior = m_pBehavior) {
        value = pBehavior->valueToParameter(value);
    }
    return value;
}

double ControlDoublePrivate::getParameterForMidiValue(double midiValue) const
{
    if(auto pBehavior = m_pBehavior) {
        return pBehavior->midiValueToParameter(midiValue);
    }
    return midiValue;
}

void ControlDoublePrivate::setMidiParameter(MidiOpCode opcode, double dParam)
{
    if(auto pBehavior = m_pBehavior){
        pBehavior->setValueFromMidiParameter(opcode, dParam, this);
    } else {
        set(dParam, NULL);
    }
}

double ControlDoublePrivate::getMidiParameter() const {
    double value = get();
    if(auto pBehavior = m_pBehavior) {
        value = pBehavior->valueToMidiParameter(value);
    }
    return value;
}

bool ControlDoublePrivate::connectValueChangeRequest(const QObject* receiver,
        const char* method, Qt::ConnectionType type)
{
    // confirmation is only required if connect was successful
    if(auto res = connect(this, SIGNAL(valueChangeRequest(double)),receiver, method, type)) {
        return(m_confirmRequired = true);
    }
    return false;
}

void ControlDoublePrivate::setUserConfig(UserSettingsPointer pConfig)
{
    s_pUserConfig = pConfig;
}
QString ControlDoublePrivate::name() const
{
    return m_name;
}

void ControlDoublePrivate::setName(QString name)
{
    if(m_name != name) {
        m_name = name;
        emit nameChanged();
    }
}

QString ControlDoublePrivate::description() const
{
    return m_description;
}

void ControlDoublePrivate::setDescription(QString description)
{
    if(m_description != description) {
        m_description = description;
        emit descriptionChanged();
    }
}

double ControlDoublePrivate::get() const
{
    return m_value.load();
}
bool ControlDoublePrivate::ignoreNops() const
{
    return m_bIgnoreNops;
}

void ControlDoublePrivate::setDefaultValue(double dValue)
{
    if(dValue != m_defaultValue.exchange(dValue)) {
        emit defaultValueChanged(dValue);
    }
}
double ControlDoublePrivate::defaultValue() const
{
    return m_defaultValue.load();
}

ControlObject* ControlDoublePrivate::getCreatorCO() const
{
    return m_pCreatorCO;
}
ConfigKey ControlDoublePrivate::getKey()
{
    return m_key;
}

