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

#ifndef ENGINEFILTER_H
#define ENGINEFILTER_H

#define MIXXX
#include "engine/engineobject.h"
#include <fidlib.h>
#include <algorithm>
#include "util/types.h"

#define PREDEF_HP 1
#define PREDEF_BP 2
#define PREDEF_LP 3

class EngineFilter : public EngineObject {
    Q_OBJECT
  public:
  struct FidData {
      FidFunc *funcp = nullptr;
      void    *runp  = nullptr;
    };
    EngineFilter(char* conf, int predefinedType = 0);
    virtual ~EngineFilter();
    void process(CSAMPLE* pInOut, const int iBufferSize);
  protected:
#define FILTER_BUF_SIZE 16
    CSAMPLE buf1[FILTER_BUF_SIZE];
    CSAMPLE buf2[FILTER_BUF_SIZE];
  private:
    CSAMPLE (*processSample)(void *buf, const CSAMPLE sample);
    FidFilter *ff  = nullptr;
    FidFunc *funcp = nullptr;
    FidRun *run = nullptr;
    void *fbuf1 = nullptr;
    void *fbuf2 = nullptr;
};
CSAMPLE processSampleDynamic(void *buf, const CSAMPLE  sample);
CSAMPLE processSampleHp(void *buf, const CSAMPLE  sample);
CSAMPLE processSampleBp(void *buf, const CSAMPLE  sample);
CSAMPLE processSampleLp(void *buf, const CSAMPLE  sample);
#endif
