/***************************************************************************
                          engineobject.cpp  -  description
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

#include "engineobject.h"

EngineObject::EngineObject(QString group, QObject *p)
  : QObject(p)
  , m_group(group)
{
}
EngineObject::~EngineObject() = default;
void EngineObject::collectFeatures(GroupFeatureState* /*pGroupFeatures*/) const
{
}
QString EngineObject::getGroup() const
{
  return m_group;
}
EngineObjectConstIn::EngineObjectConstIn(QString group, QObject *p)
:QObject(p)
,m_group(group)
{
}
EngineObjectConstIn::~EngineObjectConstIn() = default;
QString EngineObjectConstIn::getGroup() const
{
  return m_group;
}
