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
#include <cstring>
#include <cstring>
#include <atomic>
#include <memory>
#include "util/types.h"
#include "engine/effects/groupfeaturestate.h"
/**
  *@author Tue and Ken Haste Andersen
  */

class EngineObject : public QObject {
    Q_OBJECT
    int recursion_depth = 0;
  public:
    EngineObject( QObject *pParent );
    virtual ~EngineObject();
    virtual void process(CSAMPLE* pInOut, const int iBufferSize)
    {
      RELEASE_ASSERT(recursion_depth<2);
      recursion_depth++;
      auto tmp = std::make_unique<CSAMPLE[]>(iBufferSize);
      std::memmove(tmp.get(),pInOut,iBufferSize*sizeof(CSAMPLE));
      process(tmp.get(),pInOut, iBufferSize);
      recursion_depth--;
    };
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOut, const int iBufferSize)
    {
      RELEASE_ASSERT(recursion_depth<2);
      recursion_depth++;
      if(pOut!=pIn){
        std::memmove(pOut,pIn,sizeof(CSAMPLE)*iBufferSize);
      }
      process(pOut,iBufferSize);
      recursion_depth--;
    };
    // Sub-classes re-implement and populate GroupFeatureState with the features
    // they extract.
    virtual void collectFeatures(GroupFeatureState* pGroupFeatures) const {Q_UNUSED(pGroupFeatures);}
};
#endif
