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

_Pragma("once")
#include <QObject>

#include "util/types.h"
#include "engine/effects/groupfeaturestate.h"

/**
  *@author Tue and Ken Haste Andersen
  */

class EngineObject : public QObject {
    Q_OBJECT
  public:
    EngineObject(QString group, QObject *pParent );
    virtual ~EngineObject();
    virtual void process(CSAMPLE* pInOut, const int iBufferSize) = 0;
    // Sub-classes re-implement and populate GroupFeatureState with the features
    // they extract.
    virtual void collectFeatures(GroupFeatureState* pGroupFeatures) const;
    virtual QString getGroup() const;
  protected:
    const QString m_group;
};
class EngineObjectConstIn : public QObject {
    Q_OBJECT
  public:
    EngineObjectConstIn( QString group, QObject *pParent);
    virtual ~EngineObjectConstIn();
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOut, const int iBufferSize) = 0;
    virtual QString getGroup() const;
  protected:
    const QString m_group;

};
