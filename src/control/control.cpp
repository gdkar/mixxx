#include <QtDebug>
#include <QMutexLocker>

#include "control/control.h"

#include "util/stat.h"
#include "util/timer.h"

// Static member variable definition
ConfigObject<ConfigValue>* ControlValueDouble::s_pUserConfig = NULL;
QHash<ConfigKey, QWeakPointer<ControlValueDouble> > ControlValueDouble::s_qCOHash;
QHash<ConfigKey, ConfigKey> ControlValueDouble::s_qCOAliasHash;
QMutex ControlValueDouble::s_qCOHashMutex;

/*
ControlValueDouble::ControlValueDouble()
        : m_bIgnoreNops(true),
          m_bTrack(false),
          m_trackType(Stat::UNSPECIFIED),
          m_trackFlags(Stat::COUNT | Stat::SUM | Stat::AVERAGE |
                       Stat::SAMPLE_VARIANCE | Stat::MIN | Stat::MAX),
          m_confirmRequired(false) {
    initialize();
}
*/

ControlValueDouble::ControlValueDouble(ConfigKey key,
                                           ControlObject* pCreatorCO,
                                           bool bIgnoreNops, bool bTrack,
                                           double defaultValue, bool bPersist)
        : m_key(key),
          m_bPersistInConfiguration(bPersist),
          m_bIgnoreNops(bIgnoreNops),
          m_bTrack(bTrack),
          m_trackKey("control " + m_key.group + "," + m_key.item),
          m_trackType(Stat::UNSPECIFIED),
          m_trackFlags(Stat::COUNT | Stat::SUM | Stat::AVERAGE |
                       Stat::SAMPLE_VARIANCE | Stat::MIN | Stat::MAX),
          m_confirmRequired(false),
          m_pCreatorCO(pCreatorCO) {
    initialize(defaultValue);
}

void ControlValueDouble::initialize(double defaultValue) {
    m_defaultValue.setValue(defaultValue);
    double initialValue = defaultValue;
    if (m_bPersistInConfiguration) {
        ConfigObject<ConfigValue>* pConfig = ControlValueDouble::s_pUserConfig;
        if (pConfig != NULL) {
            // Assume toDouble() returns 0 if conversion fails.
            initialValue = pConfig->getValueString(m_key).toDouble();
        }
    }
    m_value.setValue(initialValue);

    //qDebug() << "Creating:" << m_trackKey << "at" << &m_value << sizeof(m_value);

    if (m_bTrack) {
        // TODO(rryan): Make configurable.
        Stat::track(m_trackKey, static_cast<Stat::StatType>(m_trackType),
                    static_cast<Stat::ComputeFlags>(m_trackFlags),
                    m_value.getValue());
    }
}

ControlValueDouble::~ControlValueDouble() {
    s_qCOHashMutex.lock();
    //qDebug() << "ControlValueDouble::s_qCOHash.remove(" << m_key.group << "," << m_key.item << ")";
    s_qCOHash.remove(m_key);
    s_qCOHashMutex.unlock();

    if (m_bPersistInConfiguration) {
        ConfigObject<ConfigValue>* pConfig = ControlValueDouble::s_pUserConfig;
        if (pConfig != NULL) {
            pConfig->set(m_key, QString::number(get()));
        }
    }
}

// static
void ControlValueDouble::insertAlias(const ConfigKey& alias, const ConfigKey& key) {
    QMutexLocker locker(&s_qCOHashMutex);

    QHash<ConfigKey, QWeakPointer<ControlValueDouble> >::const_iterator it =
            s_qCOHash.find(key);
    if (it == s_qCOHash.end()) {
        qWarning() << "WARNING: ControlValueDouble::insertAlias called for null control" << key;
        return;
    }

    QSharedPointer<ControlValueDouble> pControl = it.value();
    if (pControl.isNull()) {
        qWarning() << "WARNING: ControlValueDouble::insertAlias called for expired control" << key;
        return;
    }

    s_qCOAliasHash.insert(key, alias);
    s_qCOHash.insert(alias, pControl);
}

// static
QSharedPointer<ControlValueDouble> ControlValueDouble::getControl(
        const ConfigKey& key, bool warn, ControlObject* pCreatorCO,
        bool bIgnoreNops, bool bTrack, double defaultValue, bool bPersist) {
    if (key.isNull()) {
        if (warn) {
            qWarning() << "ControlValueDouble::getControl returning NULL"
                       << "for empty ConfigKey.";
        }
        return QSharedPointer<ControlValueDouble>();
    }

    QMutexLocker locker(&s_qCOHashMutex);
    QSharedPointer<ControlValueDouble> pControl;
    QHash<ConfigKey, QWeakPointer<ControlValueDouble> >::const_iterator it = s_qCOHash.find(key);

    if (it != s_qCOHash.end()) {
        if (pCreatorCO) {
            if (warn) {
                qDebug() << "ControlObject" << key.group << key.item << "already created";
            }
        } else {
            pControl = it.value();
        }
    }

    locker.unlock();

    if (pControl == NULL) {
        if (pCreatorCO) {
            pControl = QSharedPointer<ControlValueDouble>(
                    new ControlValueDouble(key, pCreatorCO, bIgnoreNops,
                                             bTrack, defaultValue, bPersist));
            locker.relock();
            //qDebug() << "ControlValueDouble::s_qCOHash.insert(" << key.group << "," << key.item << ")";
            s_qCOHash.insert(key, pControl);
            locker.unlock();
        } else if (warn) {
            qWarning() << "ControlValueDouble::getControl returning NULL for ("
                       << key.group << "," << key.item << ")";
        }
    }
    return pControl;
}

// static
void ControlValueDouble::getControls(
        QList<QSharedPointer<ControlValueDouble> >* pControlList) {
    s_qCOHashMutex.lock();
    pControlList->clear();
    for (QHash<ConfigKey, QWeakPointer<ControlValueDouble> >::const_iterator it = s_qCOHash.begin();
             it != s_qCOHash.end(); ++it) {
        QSharedPointer<ControlValueDouble> pControl = it.value();
        if (!pControl.isNull()) {
            pControlList->push_back(pControl);
        }
    }
    s_qCOHashMutex.unlock();
}

// static
QHash<ConfigKey, ConfigKey> ControlValueDouble::getControlAliases() {
    QMutexLocker locker(&s_qCOHashMutex);
    return s_qCOAliasHash;
}

void ControlValueDouble::reset() {
    double defaultValue = m_defaultValue.getValue();
    // NOTE: pSender = NULL is important. The originator of this action does
    // not know the resulting value so it makes sense that we should emit a
    // general valueChanged() signal even though we know the originator.
    set(defaultValue, NULL);
}
void ControlValueDouble::set(double value){
  QSharedPointer<ControlNumericBehavior > pBehavior = m_pBehavior;
  if(value==get() || (!pBehavior.isNull() && !pBehavior->setFilter(&value)))
    return;
  if ( m_confirmRequired )
    emit(valueChangeRequest(value));
  else
    setInner(value);
}
void ControlValueDouble::set(double value, QObject* pSender) {
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

void ControlValueDouble::setAndConfirm(double value, QObject* pSender) {
    setInner(value, pSender);
}
void ControlValueDouble::setInner(double value) {
    if (m_bIgnoreNops && get() == value) {
        return;
    }
    m_value.setValue(value);
    emit(valueChanged(value));

    if (m_bTrack) {
        Stat::track(m_trackKey, static_cast<Stat::StatType>(m_trackType),
                    static_cast<Stat::ComputeFlags>(m_trackFlags), value);
    }
}


void ControlValueDouble::setInner(double value, QObject* pSender) {
    if (m_bIgnoreNops && get() == value) {
        return;
    }
    m_value.setValue(value);
    emit(valueChanged(value, pSender));

    if (m_bTrack) {
        Stat::track(m_trackKey, static_cast<Stat::StatType>(m_trackType),
                    static_cast<Stat::ComputeFlags>(m_trackFlags), value);
    }
}

void ControlValueDouble::setBehavior(ControlNumericBehavior* pBehavior) {
    // This marks the old mpBehavior for deletion. It is deleted once it is not
    // used in any other function
    m_pBehavior = QSharedPointer<ControlNumericBehavior>(pBehavior);
}

void ControlValueDouble::setParameter(double dParam, QObject* pSender) {
    QSharedPointer<ControlNumericBehavior> pBehavior = m_pBehavior;
    if (pBehavior.isNull()) {
        set(dParam, pSender);
    } else {
        pBehavior->setValueFromParameter(dParam,this);
    }
}

double ControlValueDouble::getParameter() const {
    return getParameterForValue(get());
}

double ControlValueDouble::getParameterForValue(double value) const {
    QSharedPointer<ControlNumericBehavior> pBehavior = m_pBehavior;
    if (!pBehavior.isNull()) {
        value = pBehavior->valueToParameter(value);
    }
    return value;
}

bool ControlValueDouble::connectValueChangeRequest(const QObject* receiver,
        const char* method, Qt::ConnectionType type) {
    // confirmation is only required if connect was successful
    m_confirmRequired = connect(this, SIGNAL(valueChangeRequest(double)),
                receiver, method, type);
    return m_confirmRequired;
}
