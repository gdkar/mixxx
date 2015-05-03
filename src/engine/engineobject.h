/***************************************************************************
                          engineobject.h  -  description
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

#ifndef ENGINEOBJECT_H
#define ENGINEOBJECT_H

#include <QObject>

#include "util/types.h"
#include "engine/effects/groupfeaturestate.h"

/**
  *@author Tue and Ken Haste Andersen
  */

class EngineObject : public QObject {
    Q_OBJECT;
  public:
    explicit EngineObject( QObject *pParent=0);
    virtual ~EngineObject();
    Q_INVOKABLE virtual void process(CSAMPLE* pInOut,const int iBufferSize) = 0;
    // Sub-classes re-implement and populate GroupFeatureState with the features
    // they extract.
    Q_INVOKABLE virtual void collectFeatures(GroupFeatureState* ) const {}
};

class EngineObjectConstIn : public QObject {
    Q_OBJECT;
  public:
    explicit EngineObjectConstIn(QObject *pParent=0);
    virtual ~EngineObjectConstIn();
    Q_INVOKABLE virtual void process(const CSAMPLE* pIn, CSAMPLE* pOut,
                         const int iBufferSize) = 0;
};

#endif
