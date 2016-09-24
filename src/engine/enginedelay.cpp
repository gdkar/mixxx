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

#include "enginedelay.h"

#include "control/controlproxy.h"
#include "control/controlobject.h"
#include "util/assert.h"
#include "util/sample.h"

const int kiMaxDelay = 40000; // 208 ms @ 96 kb/s
const double kdMaxDelayPot = 200; // 200 ms

EngineDelay::EngineDelay(QObject *p, const char* group, ConfigKey delayControl)
        : EngineObject(p),
          m_iDelayPos(0),
          m_iDelay(0) {
    m_pDelayBuffer = SampleUtil::alloc(kiMaxDelay);
    SampleUtil::clear(m_pDelayBuffer, kiMaxDelay);
    m_pDelayPot = new ControlObject(delayControl,this);
    m_pDelayPot->setDefaultValue(0);
    connect(m_pDelayPot, &ControlObject::valueChanged, this,
            &EngineDelay::onDelayChanged, Qt::AutoConnection);
    m_pSampleRate = new ControlProxy(group, "samplerate", this);
    m_pSampleRate->connectValueChanged(SLOT(slotDelayChanged()), Qt::AutoConnection);
}

EngineDelay::~EngineDelay()
{
    SampleUtil::free(m_pDelayBuffer);
}
void EngineDelay::onDelayChanged() {
    auto newDelay = m_pDelayPot->get();
    auto sampleRate = m_pSampleRate->get();

    m_iDelay = (int)(sampleRate * newDelay / 1000);
    m_iDelay *= 2;
    if (m_iDelay > (kiMaxDelay - 2)) {
        m_iDelay = (kiMaxDelay - 2);
    }
    if (m_iDelay <= 0) {
        // We start bypassing, so clear buffer, to avoid noise in case of re-enable delay
        SampleUtil::clear(m_pDelayBuffer, kiMaxDelay);
    }
}


void EngineDelay::process(CSAMPLE* pInOut, const int iBufferSize)
{
    if (m_iDelay > 0) {
        auto iDelaySourcePos = (m_iDelayPos + kiMaxDelay - m_iDelay) % kiMaxDelay;
        DEBUG_ASSERT_AND_HANDLE(iDelaySourcePos >= 0) {
            return;
        }
        DEBUG_ASSERT_AND_HANDLE(iDelaySourcePos <= kiMaxDelay) {
            return;
        }
        for (int i = 0; i < iBufferSize; ++i) {
            // put sample into delay buffer:
            m_pDelayBuffer[m_iDelayPos] = pInOut[i];
            m_iDelayPos = (m_iDelayPos + 1) % kiMaxDelay;
            // Take delayed sample from delay buffer and copy it to dest buffer:
            pInOut[i] = m_pDelayBuffer[iDelaySourcePos];
            iDelaySourcePos = (iDelaySourcePos + 1) % kiMaxDelay;
        }
    }
}
