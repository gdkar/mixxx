/***************************************************************************
 *      enginefilter.cpp - Wrapper for FidLib Filter Library               *
 *          ----------------------                             *
 *   copyright      : (C) 2007 by John Sully                               *
 *   email          : jsully@scs.ryerson.ca                                *
 *                                                                         *
 **************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "engine/enginefilter.h"
#include <QtDebug>


EngineFilter::EngineFilter(char * conf)
        : ff(std::unique_ptr<FidFilter>(fid.design(conf,44100.,-1.,-1.,1,nullptr)))
        , run(std::make_unique<Run<CSAMPLE> >(ff))
        , buf{std::make_unique<RunBuf<CSAMPLE> >(run),
              std::make_unique<RunBuf<CSAMPLE> >(run)}
{
}

EngineFilter::~EngineFilter() = default;
void EngineFilter::process(CSAMPLE* pInOut, const int iBufferSize)
{
    for(auto i = 0; i < iBufferSize; i += 2)
    {
        pInOut[i + 0] = buf[0]->step(pInOut[i + 0]);
        pInOut[i + 1] = buf[1]->step(pInOut[i + 1]);
    }
}
