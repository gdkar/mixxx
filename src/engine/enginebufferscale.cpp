/***************************************************************************
                          enginebufferscale.cpp  -  description
                             -------------------
    begin                : Sun Apr 13 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "engine/enginebufferscale.h"
#include "engine/readaheadmanager.h"
#include "util/defs.h"
#include "sampleutil.h"

EngineBufferScale::EngineBufferScale(ReadAheadManager * pRAMan, QObject *pParent)
        : QObject(pParent),
          m_pRAMan(pRAMan),
          m_buffer(std::make_unique<CSAMPLE[]>(MAX_BUFFER_LEN))
{
}

EngineBufferScale::~EngineBufferScale()
{
}

double EngineBufferScale::getSamplesRead()
{
    return m_samplesRead;
}
void EngineBufferScale::setSampleRate(int iSampleRate)
{
  m_iSampleRate = iSampleRate;
}
void EngineBufferScale::setScaleParameters(double base_rate,double* pTempoRatio,double* pPitchRatio)
{
    m_dBaseRate = base_rate;
    m_dTempoRatio = *pTempoRatio;
    m_dPitchRatio = *pPitchRatio;
}
