/***************************************************************************
                          controlobject.cpp  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QtDebug>
#include <QHash>
#include <QSet>
#include <QMutexLocker>

#include "control/controlobject.h"
#include "control/control.h"
#include "util/stat.h"
#include "util/timer.h"

ControlObject::ControlObject(QObject *p) : QObject(p)
{
    if(auto pControl = qobject_cast<ControlDoublePrivate*>(p)) {
        m_pControl = pControl->sharedFromThis();
        connect(m_pControl.data(), &ControlDoublePrivate::valueChanged,
                this, &ControlObject::privateValueChanged,
                static_cast<Qt::ConnectionType>(Qt::AutoConnection| Qt::UniqueConnection)
                );
        connect(m_pControl.data(), &ControlDoublePrivate::defaultValueChanged,
                this, &ControlObject::defaultValueChanged,
                static_cast<Qt::ConnectionType>(Qt::AutoConnection| Qt::UniqueConnection)
                );

        connect(
            m_pControl.data()
            ,&ControlDoublePrivate::trigger
            , this
            ,&ControlObject::triggered
            , static_cast<Qt::ConnectionType>(Qt::AutoConnection| Qt::UniqueConnection
                )
            );
        connect(
            m_pControl.data()
            ,&ControlDoublePrivate::nameChanged
            , this
            ,&ControlObject::nameChanged
            , static_cast<Qt::ConnectionType>(
                Qt::AutoConnection | Qt::UniqueConnection)
            );
        connect(
            m_pControl.data()
            ,&ControlDoublePrivate::descriptionChanged
            , this
            ,&ControlObject::descriptionChanged
            , static_cast<Qt::ConnectionType>(
                Qt::AutoConnection | Qt::UniqueConnection)
            );
        m_key = m_pControl->getKey();
        m_wControl = m_pControl.toWeakRef();
//        m_pControl.reset();
    }
}

QSharedPointer<ControlDoublePrivate> ControlObject::control() const
{
    if(m_pControl)
        return m_pControl;
    return m_wControl.lock();
}
ControlObject::ControlObject(ConfigKey key, QObject *pParent, bool bIgnoreNops, bool bTrack,
                             bool bPersist)
:ControlObject(pParent)
{
    initialize(key, bIgnoreNops, bTrack, bPersist);
}

ControlObject::~ControlObject() = default;
void ControlObject::trigger()
{
    if(auto co = control())
        co->trigger();
}
void ControlObject::initialize(ConfigKey key, bool bIgnoreNops, bool bTrack,
                               bool bPersist)
{
    m_key = key;
    // Don't bother looking up the control if key is NULL. Prevents log spew.
    if (!m_key.isNull()) {
            m_pControl = ControlDoublePrivate::getControl(m_key, true,bIgnoreNops, bTrack,bPersist);

        // getControl can fail and return a NULL control even with the create flag.
        if (m_pControl) {
            connect(m_pControl.data(), &ControlDoublePrivate::valueChanged,
                    this, &ControlObject::privateValueChanged,
                    static_cast<Qt::ConnectionType>(Qt::AutoConnection| Qt::UniqueConnection)
                    );
            connect(m_pControl.data(), &ControlDoublePrivate::defaultValueChanged,
                    this, &ControlObject::defaultValueChanged,
                    static_cast<Qt::ConnectionType>(Qt::AutoConnection| Qt::UniqueConnection)
                    );

            connect(
                m_pControl.data()
                ,&ControlDoublePrivate::trigger
                , this
                ,&ControlObject::triggered
                , static_cast<Qt::ConnectionType>(Qt::AutoConnection| Qt::UniqueConnection
                    )
                );
            connect(
                m_pControl.data()
                ,&ControlDoublePrivate::nameChanged
                , this
                ,&ControlObject::nameChanged
                , static_cast<Qt::ConnectionType>(
                    Qt::AutoConnection | Qt::UniqueConnection)
                );
            connect(
                m_pControl.data()
                ,&ControlDoublePrivate::descriptionChanged
                , this
                ,&ControlObject::descriptionChanged
                , static_cast<Qt::ConnectionType>(
                    Qt::AutoConnection | Qt::UniqueConnection)
                );
        }
    }
}
// slot
void ControlObject::privateValueChanged(double dValue, QObject* pSender)
{
    // Only emit valueChanged() if we did not originate this change.
    if (pSender != this) {
        emit(valueChanged(dValue));
    } else {
        emit(valueChangedFromEngine(dValue));
    }
}

// static
ControlObject* ControlObject::getControl(ConfigKey key, bool warn)
{
    //qDebug() << "ControlObject::getControl for (" << key.group << "," << key.item << ")";
    if(!warn) {
        if(auto pCDP = ControlDoublePrivate::getIfExists(key)) {
            return pCDP->getCreatorCO();
        }
    }else{
        if(auto pCDP = ControlDoublePrivate::getControl(key)) {
            return pCDP->getCreatorCO();
        }
    }
    return NULL;
}

void ControlObject::setValueFromMidi(MidiOpCode o, double v)
{
    if (auto co = control()) {
        co->setMidiParameter(o, v);
    }
}

double ControlObject::getMidiParameter() const {
    if(auto co = control())
        return co->getMidiParameter();
    return 0.;
}

// static
double ControlObject::get(ConfigKey key) {
    if(auto pCop = ControlDoublePrivate::getIfExists(key))
        return pCop->get();
    else
        return 0.0;
}

double ControlObject::getParameter() const
{
    if(auto co = control())
        return co->getParameter();
    return 0.0;
}

double ControlObject::getParameterForValue(double value) const
{
    if(auto co = control())
        return co->getParameterForValue(value);
    return 0;
    return m_pControl ? m_pControl->getParameterForValue(value) : 0.0;
}

double ControlObject::getParameterForMidiValue(double midiValue) const {
    if(auto co = control())
        co->getParameterForMidiValue(midiValue);
    return 0.0;
}

void ControlObject::setParameter(double v) {
    if (auto co = control()) {
        co->setParameter(v, this);
    }
}

void ControlObject::setParameterFrom(double v, QObject* pSender) {
    if (auto co = control()) {
        co->setParameter(v, pSender);
    }
}

// static
void ControlObject::set(ConfigKey key, double value) {
    if(auto pCop = ControlDoublePrivate::getIfExists(key)) {
        pCop->set(value, NULL);
    }
}

bool ControlObject::connectValueChangeRequest(const QObject* receiver,
                                              const char* method,
                                              Qt::ConnectionType type)
{
    bool ret = false;
    if (m_pControl) {
        ret = m_pControl->connectValueChangeRequest(receiver, method, type);
    }
    return ret;
}
ControlObject* ControlObject::getControl(QString group, QString item, bool warn ) {
    ConfigKey key(group, item);
    return getControl(key, warn);
}
ControlObject* ControlObject::getControl(const char* group, const char* item, bool warn ) {
    ConfigKey key(group, item);
    return getControl(key, warn);
}

QString ControlObject::name() const {
    return m_pControl ?  m_pControl->name() : QString();
}

void ControlObject::setName(QString name) {
    if (auto co = control()) {
        co->setName(name);
    }
}

QString ControlObject::description() const
{
    if(auto co = control())
        return co->description();
    else
        return QString{};
}

void ControlObject::setDescription(QString description) {
    if (auto co = control()) {
        co->setDescription(description);
    }
}

// Return the key of the object
ConfigKey ControlObject::getKey() const {
    return m_key;
}

// Returns the value of the ControlObject
double ControlObject::get() const {
    if(auto co = control())
        return co->get();
    return 0;
}

// Returns the bool interpretation of the ControlObject
bool ControlObject::toBool() const {
    return get() > 0.0;
}
void ControlObject::set(double value) {
    if(auto co = control())
        co->set(value,this);
}
// Sets the ControlObject value and confirms it.
void ControlObject::setAndConfirm(double value) {
    if (auto co = control()) {
        co->setAndConfirm(value, this);
    }
}
void ControlObject::reset() {
    if (auto co = control()) {
        co->reset();
    }
}
void ControlObject::setDefaultValue(double dValue)
{
    if (auto co = control()) {
        co->setDefaultValue(dValue);
    }
}
double ControlObject::defaultValue() const
{
    if(auto co = control()) return co->defaultValue();
    return 0;
}
bool ControlObject::ignoreNops() const
{
    if(auto co = control())
        return co->ignoreNops();
    return true;
}
double ControlObject::operator += ( double incr)
{
    if(m_pControl) {
        return m_pControl->updateAtomically([incr](double x){return x + incr;});
    }
    return 0;
}
double ControlObject::operator -= ( double incr)
{
    if(m_pControl) {
        return m_pControl->updateAtomically([incr](double x){return x - incr;});
    }
    return 0;
}
double ControlObject::operator ++ (int)
{
    if(m_pControl) {
        return m_pControl->updateAtomically([](double x){return x + 1;});
    }
    return 0;
}
double ControlObject::operator -- (int)
{
    if(m_pControl) {
        return m_pControl->updateAtomically([](double x){return x - 1;});
    }
    return 0;
}
double ControlObject::operator ++ ()
{
    return (++(*this))+1;
}
double ControlObject::operator -- ()
{
    return (--(*this))-1;
}
double ControlObject::increment(double x)
{
    return (((*this)+=x));
}
double ControlObject::decrement(double x)
{
    return (((*this)-=x));
}
double ControlObject::fetch_add(double val)
{
    return m_pControl ? m_pControl->updateAtomically([val](double x){return x + val;}) : 0.0;
}
double ControlObject::fetch_sub(double val)
{
    return m_pControl ? m_pControl->updateAtomically([val](double x){return x - val;}) : 0.0;
}
bool ControlObject::compare_exchange(double &expected, double desired)
{
    return m_pControl ? m_pControl->compare_exchange_strong(expected,desired) : false;
}

double ControlObject::exchange(double val)
{
    return m_pControl ? m_pControl->updateAtomically([val](double ){return val;}) : 0.0;
}
double ControlObject::fetch_mul(double val)
{
    return m_pControl ? m_pControl->updateAtomically([val](double x){return val * x;}) : 0.0;
}
double ControlObject::fetch_div(double val)
{
    return m_pControl ? m_pControl->updateAtomically([val](double x){return val / x;}) : 0.0;
}
double ControlObject::fetch_toggle()
{
    return m_pControl ? m_pControl->updateAtomically([](double x){return !x;}) : 0.0;
}
