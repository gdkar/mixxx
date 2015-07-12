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

#include "engine/engineobject.h"
#include <fidlib.h>
#include <qsharedpointer.h>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include "util/types.h"

#define PREDEF_HP 1
#define PREDEF_BP 2
#define PREDEF_LP 3

class EngineFilter : public EngineObject {
    Q_OBJECT
  public:
    EngineFilter(char* conf, int predefinedType = 0, QObject *pParent=nullptr);
    virtual ~EngineFilter();
    void process(CSAMPLE* pInOut, const int iBufferSize);
  protected:
#define FILTER_BUF_SIZE 16
    CSAMPLE  buf1[FILTER_BUF_SIZE];
    CSAMPLE  buf2[FILTER_BUF_SIZE];

  private:
    struct FidData {
      static void fid_run_deleter(FidRun *run){fid_run_free(run);}
      QSharedPointer<FidFilter> filterp;
      FidFunc*                  funcp;
      QSharedPointer<FidRun>    runp;
      void*                     bufp;
      FidData() = default;
      FidData(const char *conf)
        : filterp(fid_design(conf,44100, -1., -1., 1.0, nullptr),std::free)
        , funcp(nullptr)
        , runp (nullptr)
        , bufp (nullptr){
          runp.reset(fid_run_new ( filterp.data(), &funcp ), &FidData::fid_run_deleter);
          bufp = fid_run_newbuf ( runp.data() );
        }
      FidData ( FidData &&other)
        : filterp(other.filterp)
        , funcp  (other.funcp  )
        , runp   (other.runp   )
        , bufp   (other.bufp   )
      {}
      FidData ( const FidData &other )
        : filterp(other.filterp)
        , funcp  (other.funcp  )
        , runp   (other.runp)
        , bufp   (fid_run_newbuf(other.runp.data()))
      {}
     ~FidData(){if(bufp){fid_run_freebuf(bufp);bufp=nullptr;}}
    };
    void *fbuf1;
    void *fbuf2;
    CSAMPLE  (*processSample)(void *buf, const CSAMPLE sample);
    static CSAMPLE processSampleDynamic(void *buf, const CSAMPLE sample);
    static CSAMPLE processSampleHp     (void *buf, const CSAMPLE sample);
    static CSAMPLE processSampleBp     (void *buf, const CSAMPLE sample);
    static CSAMPLE processSampleLp     (void *buf, const CSAMPLE sample);
};
#endif
