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
#include "util/assert.h"
#include "util/timer.h"

ControlObject::ControlObject(QObject *p):QObject(p)
{
};
ControlObject::ControlObject(ConfigKey key, QObject *p,  bool bTrack,bool bPersist)
  :QObject(p)
{
    initialize(key, bTrack, bPersist);
}
ControlObject::~ControlObject()
{
    if(m_pControl) m_pControl->removeCreatorCO(this);
};
void ControlObject::initialize(ConfigKey key, bool bTrack,bool bPersist)
{
    m_key = key;
    // Don't bother looking up the control if key is nullptr. Prevents log spew.
    if (!m_key.isNull()) m_pControl = ControlDoublePrivate::getControl(m_key, false, this, bTrack, bPersist);
    // getControl can fail and return a nullptr control even with the create flag.
    if (m_pControl)
    {
        auto flags = static_cast<Qt::ConnectionType>(Qt::DirectConnection|Qt::UniqueConnection);
        connect(m_pControl.data(), &ControlDoublePrivate::valueChanged,this, &ControlObject::valueChanged,flags);
        connect(m_pControl.data(), &ControlDoublePrivate::nameChanged,this, &ControlObject::nameChanged,flags);
        connect(m_pControl.data(), &ControlDoublePrivate::descriptionChanged,this, &ControlObject::descriptionChanged,flags);
        connect(m_pControl.data(), &ControlDoublePrivate::parameterChanged,this, &ControlObject::parameterChanged,flags);
    }
}
/* static */ ControlObject*
ControlObject::getControl(QString _group,QString _item, bool warn)
{
  return getControl(ConfigKey{_group,_item},warn);
}
/* static */ ControlObject*
ControlObject::getControl(const char *_group, const char *_item, bool warn)
{
  return getControl ( ConfigKey{QString{_group},QString{_item}},warn );
}
ConfigKey ControlObject::getKey()const
{
  return m_key;
}
double ControlObject::get()const
{
  if ( m_pControl ) return m_pControl->get(); else return 0;
}
bool ControlObject::toBool() const 
{
  return get()>0.0;
}
void ControlObject::set(double v)
{
  if(m_pControl)m_pControl->set(v);
}
void ControlObject::setAndConfirm(double v)
{
  if(m_pControl)m_pControl->setAndConfirm(v,nullptr);
}
void ControlObject::reset()
{
  if(m_pControl)m_pControl->reset();
}
void ControlObject::setDefaultValue(double v)
{
  if(m_pControl) m_pControl->setDefaultValue(v);
}
double ControlObject::defaultValue()const
{
  return m_pControl ? m_pControl->defaultValue():0.0;
}
ControlObject::operator bool()const
{
  return toBool();
}
bool ControlObject::operator!()const
{
  return !toBool();
}
ControlObject::operator int() const
{
  return static_cast<int>(get());
}
// static
ControlObject* ControlObject::getControl(ConfigKey key, bool warn)
{
    if(auto pCDP = ControlDoublePrivate::getControl(key, warn)) return pCDP->getCreatorCO();
    return nullptr;
}
// static
double ControlObject::get(ConfigKey key)
{
    if(auto pCop = ControlDoublePrivate::getControl(key)) return pCop->get(); else return  0.0;
}
double ControlObject::getParameter() const
{
  return m_pControl ? m_pControl->getParameter() : 0.0;
}
double ControlObject::getParameterForValue(double value) const
{
  return m_pControl ? m_pControl->getParameterForValue(value) : 0.0;
}
void ControlObject::setParameter(double v)
{ 
  if (m_pControl) m_pControl->setParameter(v);
}
// static
void ControlObject::set(ConfigKey key, double value)
{
    auto pCop = ControlDoublePrivate::getControl(key);
    if (pCop) pCop->set(value);
}
bool ControlObject::connectValueChangeRequest(const QObject* receiver,const char* method, Qt::ConnectionType type)
{
    auto ret = false;
    if (m_pControl) ret = m_pControl->connectValueChangeRequest(receiver, method, type);
    return ret;
}
QString ControlObject::name() const
{
  if(m_pControl) return m_pControl->name(); else return QString{};
}
void ControlObject::setName(QString s)
{
  if(m_pControl) m_pControl->setName(s);
}
QString ControlObject::description() const
{
  if(m_pControl) return m_pControl->description(); else return QString{};
}
void ControlObject::setDescription(QString s)
{
  if(m_pControl) m_pControl->setDescription(s);
}
QString ControlObject::group() const
{
  return m_key.group;
}
QString ControlObject::item() const
{
  return m_key.item;
}
// connect to parent object
bool ControlObject::connectValueChanged( const char* method, Qt::ConnectionType type) 
{
    if ( auto p = parent() )  connectValueChanged(p, method, type);
    return true;
}
bool ControlObject::connectValueChanged(const QObject* receiver,const char* method, Qt::ConnectionType type)
{
    if (m_pControl) connect(this, SIGNAL(valueChanged(double)),receiver, method, type);
    return true;
}
