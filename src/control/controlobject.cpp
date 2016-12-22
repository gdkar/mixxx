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

ControlObject::ControlObject() {
}

ControlObject::ControlObject(ConfigKey key, bool bIgnoreNops, bool bTrack,
                             bool bPersist, double defaultValue) {
    initialize(key, bIgnoreNops, bTrack, bPersist, defaultValue);
}

ControlObject::~ControlObject() {
    if (m_pControl) {
        m_pControl->removeCreatorCO();
    }
}

void ControlObject::initialize(ConfigKey key, bool bIgnoreNops, bool bTrack,
                               bool bPersist, double defaultValue)
{
    m_key = key;

    // Don't bother looking up the control if key is NULL. Prevents log spew.
    if (!m_key.isNull()) {
        m_pControl = ControlDoublePrivate::getControl(m_key, true, this,
                                                      bIgnoreNops, bTrack,
                                                      bPersist, defaultValue);
    }

    // getControl can fail and return a NULL control even with the create flag.
    if (m_pControl) {
        connect(m_pControl.data(), SIGNAL(valueChanged(double, QObject*)),
                this, SLOT(privateValueChanged(double, QObject*)),
                Qt::DirectConnection);
    }
}

// slot
void ControlObject::privateValueChanged(double dValue, QObject* pSender) {
    // Only emit valueChanged() if we did not originate this change.
    if (pSender != this) {
        emit(valueChanged(dValue));
    } else {
        emit(valueChangedFromEngine(dValue));
    }
}

// static
ControlObject* ControlObject::getControl(const ConfigKey& key, bool warn) {
    //qDebug() << "ControlObject::getControl for (" << key.group << "," << key.item << ")";
    QSharedPointer<ControlDoublePrivate> pCDP = ControlDoublePrivate::getControl(key, warn);
    if (pCDP) {
        return pCDP->getCreatorCO();
    }
    return NULL;
}

void ControlObject::setValueFromMidi(MidiOpCode o, double v) {
    if (m_pControl) {
        m_pControl->setMidiParameter(o, v);
    }
}

double ControlObject::getMidiParameter() const {
    return m_pControl ? m_pControl->getMidiParameter() : 0.0;
}

// static
double ControlObject::get(const ConfigKey& key) {
    QSharedPointer<ControlDoublePrivate> pCop = ControlDoublePrivate::getControl(key);
    return pCop ? pCop->get() : 0.0;
}

double ControlObject::getParameter() const {
    return m_pControl ? m_pControl->getParameter() : 0.0;
}

double ControlObject::getParameterForValue(double value) const {
    return m_pControl ? m_pControl->getParameterForValue(value) : 0.0;
}

double ControlObject::getParameterForMidiValue(double midiValue) const {
    return m_pControl ? m_pControl->getParameterForMidiValue(midiValue) : 0.0;
}

void ControlObject::setParameter(double v) {
    if (m_pControl) {
        m_pControl->setParameter(v, this);
    }
}

void ControlObject::setParameterFrom(double v, QObject* pSender) {
    if (m_pControl) {
        m_pControl->setParameter(v, pSender);
    }
}

// static
void ControlObject::set(const ConfigKey& key, const double& value) {
    QSharedPointer<ControlDoublePrivate> pCop = ControlDoublePrivate::getControl(key);
    if (pCop) {
        pCop->set(value, NULL);
    }
}

bool ControlObject::connectValueChangeRequest(const QObject* receiver,
                                              const char* method,
                                              Qt::ConnectionType type) {
    bool ret = false;
    if (m_pControl) {
        ret = m_pControl->connectValueChangeRequest(receiver, method, type);
    }
    return ret;
}

void ControlObject::setReadOnly() {
    connectValueChangeRequest(this, SLOT(readOnlyHandler(double)),
                              Qt::DirectConnection);
}

void ControlObject::readOnlyHandler(double v) {
    qWarning() << m_key << "is read-only. Ignoring set of value:" << v;
}



QString ControlObject::name() const
{
    return m_pControl ?  m_pControl->name() : QString();
}

void ControlObject::setName(const QString& name)
{
    if (m_pControl) {
        m_pControl->setName(name);
    }
}

QString ControlObject::description() const
{
    return m_pControl ?  m_pControl->description() : QString();
}

void ControlObject::setDescription(const QString& description)
{
    if (m_pControl) {
        m_pControl->setDescription(description);
    }
}

// Return the key of the object
ConfigKey ControlObject::getKey() const
{
    return m_key;
}

// Returns the value of the ControlObject
double ControlObject::get() const
{
    return m_pControl ? m_pControl->get() : 0.0;
}

// Returns the bool interpretation of the ControlObject
bool ControlObject::toBool() const
{
    return get() > 0.0;
}

// Sets the ControlObject value. May require confirmation by owner.
void ControlObject::set(double value) {
    if (m_pControl) {
        m_pControl->set(value, this);
    }
}

// Sets the ControlObject value and confirms it.
void ControlObject::setAndConfirm(double value) {
    if (m_pControl) {
        m_pControl->setAndConfirm(value, this);
    }
}

// Forces the control to 'value', regardless of whether it has a change
// request handler attached (identical to setAndConfirm).
void ControlObject::forceSet(double value)
{
    setAndConfirm(value);
}
ControlObject* ControlObject::getControl(const QString& group, const QString& item, bool warn ) {
    ConfigKey key(group, item);
    return getControl(key, warn);
}
ControlObject* ControlObject::getControl(const char* group, const char* item, bool warn ) {
    ConfigKey key(group, item);
    return getControl(key, warn);
}
void ControlObject::reset()
{
    if (m_pControl) {
        m_pControl->reset();
    }
}

void ControlObject::setDefaultValue(double dValue)
{
    if (m_pControl) {
        m_pControl->setDefaultValue(dValue);
    }
}
double ControlObject::defaultValue() const
{
    return m_pControl ? m_pControl->defaultValue() : 0.0;
}
bool ControlObject::ignoreNops() const
{
    return m_pControl ? m_pControl->ignoreNops() : true;
}
