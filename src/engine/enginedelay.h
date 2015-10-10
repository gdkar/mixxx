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

_Pragma("once")
#include <memory>
#include "engine/engineobject.h"
#include "configobject.h"
class ControlPotmeter;
class ControlObjectSlave;
class EngineDelay : public EngineObject {
    Q_OBJECT
  public:
    EngineDelay(QString group, ConfigKey delayControl,QObject*);
    virtual ~EngineDelay();
    void process(CSAMPLE* pInOut, int iBufferSize);
  public slots:
    void slotDelayChanged();
  private:
    ControlPotmeter* m_pDelayPot = nullptr;
    ControlObjectSlave* m_pSampleRate = nullptr;
    std::unique_ptr<CSAMPLE[]> m_pDelayBuffer;
    int m_iDelayPos = 0;
    int m_iDelay    = 0;
};
