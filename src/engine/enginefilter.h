/***************************************************************************
 *      enginefilter.h - Wrapper for FidLib Filter Library                 *
 *                      ----------------------                             *
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

_Pragma("once")
#include <cstdio>
#include <memory>
#include <fidlib.h>

#include "engine/engineobject.h"
#include "util/types.h"
class EngineFilter : public EngineObject {
    Q_OBJECT
    Fid fid;
  public:
    EngineFilter(char* conf);
    virtual ~EngineFilter();
    void process(CSAMPLE* pInOut, const int iBufferSize);
  private:
    double (*processSample)(void *buf, const double sample);
    std::unique_ptr<FidFilter>           ff;
    std::unique_ptr<Run<CSAMPLE> >       run;
    std::unique_ptr<RunBuf<CSAMPLE> >    buf[2];
};
