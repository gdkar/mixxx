/***************************************************************************
                          enginedelay.h  -  description
                             -------------------
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

#ifndef ENGINEDELAY_H
#define ENGINEDELAY_H

#include "engine/engineobject.h"
#include "sampleutil.h"
#include "controlobject.h"
#include "controlpotmeter.h"
#include "controlobjectslave.h"
#include "configobject.h"

class ControlPotmeter;
class ControlObjectSlave;

class EngineDelay : public EngineObject {
    Q_OBJECT
  public:
    EngineDelay(const char* group, ConfigKey delayControl, QObject *pParent=nullptr);
    virtual ~EngineDelay();
    void process(CSAMPLE* pInOut, const int iBufferSize);
  public slots:
    void onDelayChanged();
  private:
  std::unique_ptr<CSAMPLE[]> m_pDelayBuffer;
    int m_iDelayPos;
    int m_iDelay;
    ControlPotmeter         m_pDelayPot;
    ControlObjectSlave      m_pSampleRate;
};

#endif
