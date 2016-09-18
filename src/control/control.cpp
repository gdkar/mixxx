#include <QtDebug>
#include <QSharedPointer>

#include "control/control.h"

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
                                           ControlObject* pCreatorCO,
                                           bool bIgnoreNops, bool bTrack,
                                           bool bPersist)
        : m_key(key),
          m_bPersistInConfiguration(bPersist),
          m_bIgnoreNops(bIgnoreNops),
          m_bTrack(bTrack),
          m_trackType(Stat::UNSPECIFIED),
          m_trackFlags(Stat::COUNT | Stat::SUM | Stat::AVERAGE |
                       Stat::SAMPLE_VARIANCE | Stat::MIN | Stat::MAX),
          m_confirmRequired(false),
          m_pCreatorCO(pCreatorCO) {
    initialize();
}

void ControlDoublePrivate::initialize() {
    double value = 0;
    if (m_bPersistInConfiguration) {
        UserSettingsPointer pConfig = ControlDoublePrivate::s_pUserConfig;
        if (pConfig != NULL) {
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

ControlDoublePrivate::~ControlDoublePrivate() {
    s_qCOHashMutex.lock();
    //qDebug() << "ControlDoublePrivate::s_qCOHash.remove(" << m_key.group << "," << m_key.item << ")";
    s_qCOHash.remove(m_key);
    s_qCOHashMutex.unlock();

    if (m_bPersistInConfiguration) {
        UserSettingsPointer pConfig = ControlDoublePrivate::s_pUserConfig;
        if (pConfig != NULL) {
            pConfig->set(m_key, QString::number(get()));
        }
    }
}

// static
void ControlDoublePrivate::insertAlias(const ConfigKey& alias, const ConfigKey& key) {
    MMutexLocker locker(&s_qCOHashMutex);

    QHash<ConfigKey, QWeakPointer<ControlDoublePrivate> >::const_iterator it =
            s_qCOHash.find(key);
    if (it == s_qCOHash.end()) {
        qWarning() << "WARNING: ControlDoublePrivate::insertAlias called for null control" << key;
        return;
    }

    QSharedPointer<ControlDoublePrivate> pControl = it.value();
    if (pControl.isNull()) {
        qWarning() << "WARNING: ControlDoublePrivate::insertAlias called for expired control" << key;
        return;
    }

    s_qCOAliasHash.insert(key, alias);
    s_qCOHash.insert(alias, pControl);
}

// static
QSharedPointer<ControlDoublePrivate> ControlDoublePrivate::getControl(
        const ConfigKey& key, bool warn, ControlObject* pCreatorCO,
        bool bIgnoreNops, bool bTrack, bool bPersist) {
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
        QHash<ConfigKey, QWeakPointer<ControlDoublePrivate> >::const_iterator it = s_qCOHash.find(key);

        if (it != s_qCOHash.end()) {
            if (pCreatorCO) {
                if (warn) {
                    qDebug() << "ControlObject" << key.group << key.item << "already created";
                }
            } else {
                pControl = it.value();
            }
        }
    }

    if (pControl == NULL) {
        if (pCreatorCO) {
            pControl = QSharedPointer<ControlDoublePrivate>(
                    new ControlDoublePrivate(key, pCreatorCO, bIgnoreNops,
                                             bTrack, bPersist));
            MMutexLocker locker(&s_qCOHashMutex);
            //qDebug() << "ControlDoublePrivate::s_qCOHash.insert(" << key.group << "," << key.item << ")";
            s_qCOHash.insert(key, pControl);
        } else if (warn) {
            qWarning() << "ControlDoublePrivate::getControl returning NULL for ("
                       << key.group << "," << key.item << ")";
        }
    }
    return pControl;
}

// static
void ControlDoublePrivate::getControls(
        QList<QSharedPointer<ControlDoublePrivate> >* pControlList) {
    s_qCOHashMutex.lock();
    pControlList->clear();
    for (QHash<ConfigKey, QWeakPointer<ControlDoublePrivate> >::const_iterator it = s_qCOHash.begin();
             it != s_qCOHash.end(); ++it) {
        QSharedPointer<ControlDoublePrivate> pControl = it.value();
        if (!pControl.isNull()) {
            pControlList->push_back(pControl);
        }
    }
    s_qCOHashMutex.unlock();
}

// static
QHash<ConfigKey, ConfigKey> ControlDoublePrivate::getControlAliases() {
    MMutexLocker locker(&s_qCOHashMutex);
    return s_qCOAliasHash;
}

void ControlDoublePrivate::reset()
{
    auto defaultValue = m_defaultValue.load();
    // NOTE: pSender = is important. The originator of this action does
    // not know the resulting value so it makes sense that we should emit a
    // general valueChanged() signal even though we know the originator.
    set(defaultValue,nullptr);
}

void ControlDoublePrivate::set(double value, QObject* pSender) {
    // If the behavior says to ignore the set, ignore it.
    QSharedPointer<ControlNumericBehavior> pBehavior = m_pBehavior;
    if (!pBehavior.isNull() && !pBehavior->setFilter(&value)) {
        return;
    }
    if (m_confirmRequired) {
        emit(valueChangeRequest(value));
    } else {
        setInner(value, pSender);
    }
}
void ControlDoublePrivate::setAndConfirm(double value, QObject* pSender)
{
    setInner(value, pSender);
}
void ControlDoublePrivate::setInner(double value, QObject* pSender)
{
    if(value == m_value.exchange(value) && m_bIgnoreNops)
        return;

    emit(valueChanged(value, pSender));

    if (m_bTrack) {
        Stat::track(m_trackKey, static_cast<Stat::StatType>(m_trackType),
                    static_cast<Stat::ComputeFlags>(m_trackFlags), value);
    }
}

void ControlDoublePrivate::setBehavior(ControlNumericBehavior* pBehavior) {
    // This marks the old mpBehavior for deletion. It is deleted once it is not
    // used in any other function
    m_pBehavior = QSharedPointer<ControlNumericBehavior>(pBehavior);
}

void ControlDoublePrivate::setParameter(double dParam, QObject* pSender) {
    QSharedPointer<ControlNumericBehavior> pBehavior = m_pBehavior;
    if (pBehavior.isNull()) {
        set(dParam, pSender);
    } else {
        set(pBehavior->parameterToValue(dParam), pSender);
    }
}

double ControlDoublePrivate::getParameter() const {
    return getParameterForValue(get());
}

double ControlDoublePrivate::getParameterForValue(double value) const {
    QSharedPointer<ControlNumericBehavior> pBehavior = m_pBehavior;
    if (!pBehavior.isNull()) {
        value = pBehavior->valueToParameter(value);
    }
    return value;
}

double ControlDoublePrivate::getParameterForMidiValue(double midiValue) const {
    QSharedPointer<ControlNumericBehavior> pBehavior = m_pBehavior;
    if (!pBehavior.isNull()) {
        return pBehavior->midiValueToParameter(midiValue);
    }
    return midiValue;
}

void ControlDoublePrivate::setMidiParameter(MidiOpCode opcode, double dParam) {
    QSharedPointer<ControlNumericBehavior> pBehavior = m_pBehavior;
    if (!pBehavior.isNull()) {
        pBehavior->setValueFromMidiParameter(opcode, dParam, this);
    } else {
        set(dParam, nullptr);
    }
}

double ControlDoublePrivate::getMidiParameter() const {
    QSharedPointer<ControlNumericBehavior> pBehavior = m_pBehavior;
    double value = get();
    if (!pBehavior.isNull()) {
        value = pBehavior->valueToMidiParameter(value);
    }
    return value;
}

bool ControlDoublePrivate::connectValueChangeRequest(const QObject* receiver,
        const char* method, Qt::ConnectionType type) {
    // confirmation is only required if connect was successful
    m_confirmRequired = connect(this, SIGNAL(valueChangeRequest(double)),
                receiver, method, type);
    return m_confirmRequired;
}

/*static*/ void ControlDoublePrivate::setUserConfig(UserSettingsPointer pConfig)
{
    s_pUserConfig = pConfig;
}
const QString& ControlDoublePrivate::name() const
{
    return m_name;
}
void ControlDoublePrivate::setName(const QString& name)
{
    m_name = name;
}
const QString& ControlDoublePrivate::description() const
{
    return m_description;
}
void ControlDoublePrivate::setDescription(const QString& description)
{
    m_description = description;
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
    m_defaultValue.store(dValue);
}
double ControlDoublePrivate::defaultValue() const
{
    return m_defaultValue.load();
}

ControlObject* ControlDoublePrivate::getCreatorCO() const
{
    return m_pCreatorCO;
}

void ControlDoublePrivate::removeCreatorCO()
{
    m_pCreatorCO = NULL;
}
ConfigKey ControlDoublePrivate::getKey() const
{
    return m_key;
}
