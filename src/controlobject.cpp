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

#include "controlobject.h"
#include "control/control.h"
#include "util/stat.h"
#include "util/timer.h"

ControlObject::ControlObject() = default;
ControlObject::ControlObject(ConfigKey key, bool bIgnoreNops, bool bTrack,bool bPersist) {
    initialize(key, bIgnoreNops, bTrack, bPersist);
}
ControlObject::~ControlObject() = default;
void ControlObject::initialize(ConfigKey key, bool bIgnoreNops, bool bTrack,bool bPersist) {
    m_key = key;
    // Don't bother looking up the control if key is nullptr. Prevents log spew.
    if (!m_key.isNull()) {
        m_pControl = ControlDoublePrivate::getControl(m_key, false, this,bIgnoreNops, bTrack,bPersist);
    }
    // getControl can fail and return a nullptr control even with the create flag.
    if (m_pControl) {
        connect(m_pControl.data(), SIGNAL(valueChanged(double, QObject*)),this, SLOT(privateValueChanged(double, QObject*)),Qt::DirectConnection);
    }
}
/* static */ ControlObject*
ControlObject::getControl(const QString&group,const QString&item, bool warn)
{
  return getControl(ConfigKey{group,item},warn);
}
/* static */ ControlObject*
ControlObject::getControl(const char *group, const char *item, bool warn)
{
  return getControl ( ConfigKey{QString{group},QString{item}},warn );
}
QString ControlObject::name()const{ if ( m_pControl ) return m_pControl->name(); else return QString{};}
void ControlObject::setName(const QString &name){if ( m_pControl ) m_pControl->setName(name);}
QString ControlObject::description()const{if ( m_pControl ) return m_pControl->description();else return QString{};}
void ControlObject::setDescription(const QString &s){if ( m_pControl ) m_pControl->setDescription(s);}
ConfigKey ControlObject::getKey()const{return m_key;}
double ControlObject::get()const{if ( m_pControl ) return m_pControl->get(); else return 0;}
bool ControlObject::toBool() const { return get()>0.0;}
void ControlObject::set(double v){if(m_pControl)m_pControl->set(v,this);}
void ControlObject::setAndConfirm(double v){if(m_pControl)m_pControl->setAndConfirm(v,this);}
void ControlObject::reset(){if(m_pControl)m_pControl->reset();}
void ControlObject::setDefaultValue(double v){if(m_pControl) m_pControl->setDefaultValue(v);}
double ControlObject::defaultValue()const{return m_pControl ? m_pControl->defaultValue():0.0;}
ControlObject::operator bool()const{return toBool();}
bool ControlObject::operator!()const{return !toBool();}
bool ControlObject::ignoreNops()const{return m_pControl ? m_pControl->ignoreNops() : true;}
// slot
void ControlObject::privateValueChanged(double dValue, QObject* pSender) {
    // Only emit valueChanged() if we did not originate this change.
    if (pSender != this) { emit(valueChanged(dValue));}
    else { emit(valueChangedFromEngine(dValue));}
}
// static
ControlObject* ControlObject::getControl(const ConfigKey& key, bool warn) {
    //qDebug() << "ControlObject::getControl for (" << key.group << "," << key.item << ")";
    if(auto pCDP = ControlDoublePrivate::getControl(key, warn)) { return pCDP->getCreatorCO(); }
    return nullptr;
}
// static
double ControlObject::get(const ConfigKey& key) {
    if(auto pCop = ControlDoublePrivate::getControl(key)) return pCop->get(); else return  0.0;
}
double ControlObject::getParameter() const {return m_pControl ? m_pControl->getParameter() : 0.0; }
double ControlObject::getParameterForValue(double value) const {return m_pControl ? m_pControl->getParameterForValue(value) : 0.0;}
void ControlObject::setParameter(double v) { if (m_pControl) { m_pControl->setParameter(v, this);} }
void ControlObject::setParameterFrom(double v, QObject* pSender) { if (m_pControl) { m_pControl->setParameter(v, pSender); } }
// static
void ControlObject::set(const ConfigKey& key, const double& value) {
    auto pCop = ControlDoublePrivate::getControl(key);
    if (pCop) { pCop->set(value, nullptr); }
}
bool ControlObject::connectValueChangeRequest(const QObject* receiver,const char* method, Qt::ConnectionType type) {
    auto ret = false;
    if (m_pControl) { ret = m_pControl->connectValueChangeRequest(receiver, method, type); }
    return ret;
}
