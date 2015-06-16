/***************************************************************************
                          enginedelay.cpp  -  description
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
#include <memory>
#include <string.h>
#include <cstring>
#include "enginedelay.h"
#include "controlpotmeter.h"
#include "controlobjectslave.h"
#include "sampleutil.h"
#include "util/assert.h"

const int kiMaxDelay = roundUpToPowerOf2(40000); // 208 ms @ 96 kb/s
const double kdMaxDelayPot = 200; // 200 ms

EngineDelay::EngineDelay(const char* group, ConfigKey delayControl, QObject *pParent)
        : EngineObject(pParent)
        , m_pDelayBuffer(std::make_unique<CSAMPLE[]>(kiMaxDelay))
        , m_iDelayPos(0)
        , m_iDelay(0) 
        , m_pDelayPot(delayControl, 0, kdMaxDelayPot, false, true, false, true)
        , m_pSampleRate(ConfigKey(group, "samplerate"), this){
    memset(&m_pDelayBuffer[0],0, kiMaxDelay*sizeof(CSAMPLE));
    m_pDelayPot.setDefaultValue(0);
    connect(&m_pDelayPot, SIGNAL(valueChanged(double)), this,SLOT(onDelayChanged(double)), Qt::DirectConnection);
    m_pSampleRate.connectValueChanged(SLOT(onDelayChanged()), Qt::DirectConnection);
}
EngineDelay::~EngineDelay() {}
void EngineDelay::onDelayChanged(double ) {
    double newDelay = m_pDelayPot.get();
    double sampleRate = m_pSampleRate.get();
    m_iDelay = (int)(sampleRate * newDelay / 1000);
    m_iDelay *= 2;
    if (m_iDelay > (kiMaxDelay - 2)) {m_iDelay = (kiMaxDelay - 2);}
    if (m_iDelay <= 0) {
        // We start bypassing, so clear buffer, to avoid noise in case of re-enable delay
        memset(&m_pDelayBuffer[0],0, kiMaxDelay*sizeof(CSAMPLE));
    }
}
void EngineDelay::process(CSAMPLE* pInOut, const int iBufferSize) {
    if (m_iDelay > 0) {
        int iDelaySourcePos = (m_iDelayPos + kiMaxDelay - m_iDelay) & (kiMaxDelay-1);
        DEBUG_ASSERT_AND_HANDLE(iDelaySourcePos >= 0) {return;}
        DEBUG_ASSERT_AND_HANDLE(iDelaySourcePos <= kiMaxDelay) {return;}
        for (int i = 0; i < iBufferSize; ++i) {
            // put sample into delay buffer:
            m_pDelayBuffer[m_iDelayPos] = pInOut[i];
            m_iDelayPos = (m_iDelayPos + 1) & (kiMaxDelay-1);
            // Take delayed sample from delay buffer and copy it to dest buffer:
            pInOut[i] = m_pDelayBuffer[iDelaySourcePos];
            iDelaySourcePos = (iDelaySourcePos + 1) & (kiMaxDelay-1);
        }
    }
}
