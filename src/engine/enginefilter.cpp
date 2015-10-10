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
namespace{
    CSAMPLE processSampleDynamic(void *bufIn, const CSAMPLE sample )
    {
      auto buf = reinterpret_cast<EngineFilter::FidData *>(bufIn);
      return static_cast<CSAMPLE>(buf->funcp(buf->runp,static_cast<double>(sample)));
    }
    // 250Hz-3Khz Butterworth
    CSAMPLE processSampleBp(void *bufIn, const CSAMPLE sample)
    {
      CSAMPLE *buf = (CSAMPLE*) bufIn;
      CSAMPLE val = sample;
      CSAMPLE tmp, fir, iir;
      tmp= buf[0]; memmove(buf, buf+1, 15*sizeof(CSAMPLE));
      // use 8.73843261546594e-007 below for unity gain at 100% level
      iir= val * 8.738432615466217e-007f;
      iir -= 0.8716357571117795f*tmp; fir= tmp;
      iir -= -1.704721971813985f*buf[0]; fir += -buf[0]-buf[0];
      fir += iir;
      tmp= buf[1]; buf[1]= iir; val= fir;
      iir= val;
      iir -= 0.9881828644977958f*tmp; fir= tmp;
      iir -= -1.986910175866046f*buf[2]; fir += -buf[2]-buf[2];
      fir += iir;
      tmp= buf[3]; buf[3]= iir; val= fir;
      iir= val;
      iir -= 0.6739219969192579f*tmp; fir= tmp;
      iir -= -1.534687501075499f*buf[4]; fir += -buf[4]-buf[4];
      fir += iir;
      tmp= buf[5]; buf[5]= iir; val= fir;
      iir= val;
      iir -= 0.9644444065027918f*tmp; fir= tmp;
      iir -= -1.963091971649609f*buf[6]; fir += -buf[6]-buf[6];
      fir += iir;
      tmp= buf[7]; buf[7]= iir; val= fir;
      iir= val;
      iir -= 0.5508744524070673f*tmp; fir= tmp;
      iir -= -1.437951258090782f*buf[8]; fir += buf[8]+buf[8];
      fir += iir;
      tmp= buf[9]; buf[9]= iir; val= fir;
      iir= val;
      iir -= 0.940359918647641f*tmp; fir= tmp;
      iir -= -1.938825711089784f*buf[10]; fir += buf[10]+buf[10];
      fir += iir;
      tmp= buf[11]; buf[11]= iir; val= fir;
      iir= val;
      iir -= 0.494560064204263f*tmp; fir= tmp;
      iir -= -1.400685123336887f*buf[12]; fir += buf[12]+buf[12];
      fir += iir;
      tmp= buf[13]; buf[13]= iir; val= fir;
      iir= val;
      iir -= 0.9201106143536053f*tmp; fir= tmp;
      iir -= -1.918341654664469f*buf[14]; fir += buf[14]+buf[14];
      fir += iir;
      buf[15]= iir; val= fir;
      return val;
    }

    //3Khz butterworth
    CSAMPLE processSampleHp(void *bufIn, const CSAMPLE sample)
    {
        CSAMPLE *buf = (CSAMPLE*) bufIn;
        CSAMPLE val = sample;
        CSAMPLE tmp, fir, iir;
      tmp= buf[0]; memmove(buf, buf+1, 7*sizeof(CSAMPLE));
      // use 0.3307380993576275 below for unity gain at 100% level
      iir= val * 0.3307380993576274f;
      iir -= 0.8503595356078639f*tmp; fir= tmp;
      iir -= -1.683892145680763f*buf[0]; fir += -buf[0]-buf[0];
      fir += iir;
      tmp= buf[1]; buf[1]= iir; val= fir;
      iir= val;
      iir -= 0.6256182051727028f*tmp; fir= tmp;
      iir -= -1.479369644054996f*buf[2]; fir += -buf[2]-buf[2];
      fir += iir;
      tmp= buf[3]; buf[3]= iir; val= fir;
      iir= val;
      iir -= 0.4873536896159359f*tmp; fir= tmp;
      iir -= -1.35354408027022f*buf[4]; fir += -buf[4]-buf[4];
      fir += iir;
      tmp= buf[5]; buf[5]= iir; val= fir;
      iir= val;
      iir -= 0.4219026276867782f*tmp; fir= tmp;
      iir -= -1.2939813158517f*buf[6]; fir += -buf[6]-buf[6];
      fir += iir;
      buf[7]= iir; val= fir;
      return val;
    }
    CSAMPLE processSampleLp(void *bufIn, const CSAMPLE sample)
    {
        CSAMPLE *buf = (CSAMPLE*) bufIn;
        CSAMPLE val = sample;
          CSAMPLE tmp, fir, iir;
      tmp= buf[0]; memmove(buf, buf+1, 7*sizeof(CSAMPLE));
      iir= val * 9.245468558718278e-015f;
      iir -= 0.9862009760667707f*tmp; fir= tmp;
      iir -= -1.984941152135637f*buf[0]; fir += buf[0]+buf[0];
      fir += iir;
      tmp= buf[1]; buf[1]= iir; val= fir;
      iir= val;
      iir -= 0.9611983723246083f*tmp; fir= tmp;
      iir -= -1.95995440725112f*buf[2]; fir += buf[2]+buf[2];
      fir += iir;
      tmp= buf[3]; buf[3]= iir; val= fir;
      iir= val;
      iir -= 0.9424834072512262f*tmp; fir= tmp;
      iir -= -1.941251312860088f*buf[4]; fir += buf[4]+buf[4];
      fir += iir;
      tmp= buf[5]; buf[5]= iir; val= fir;
      iir= val;
      iir -= 0.9325031355986387f*tmp; fir= tmp;
      iir -= -1.93127737157649f*buf[6]; fir += buf[6]+buf[6];
      fir += iir;
      buf[7]= iir; val= fir;
      return val;
    }
};
EngineFilter::EngineFilter(char * conf, int predefinedType, QObject *pParent)
:EngineObject(QString{},pParent)
{
    switch(predefinedType)
    {
    case PREDEF_BP:
        processSample = &processSampleBp;
        fbuf1 = buf1;
        fbuf2 = buf2;
        break;
    case PREDEF_HP:
        processSample = &processSampleHp;
        fbuf1 = buf1;
        fbuf2 = buf2;
        break;
    case PREDEF_LP:
        processSample = &processSampleLp;
        fbuf1 = buf1;
        fbuf2 = buf2;
        break;
    default:
        ff = fid_design(conf, 44100., -1., -1., 1, nullptr);
        qDebug() << "Filter " << conf << " Setup: 0x" << ff;
        run = fid_run_new(ff, &funcp);
        {
          auto data = new FidData;
          data->funcp = funcp;
          data->runp  = fid_run_newbuf(run);
          fbuf1 = data;
        }
        {
          auto data = new FidData;
          data->funcp = funcp;
          data->runp  = fid_run_newbuf(run);
          fbuf2 = data;
        }
        processSample = processSampleDynamic;
    }
    int i;
    for(i=0; i < FILTER_BUF_SIZE; i++) buf1[i] = buf2[i] = 0;
}
EngineFilter::~EngineFilter()
{
    if(processSample == processSampleDynamic) //if we used fidlib
    {
      {
        auto data = reinterpret_cast<FidData*>(fbuf2);
        fid_run_freebuf(data->runp);
        delete data;
      }
      {
        auto data = reinterpret_cast<FidData*>(fbuf1);
        fid_run_freebuf(data->runp);
        delete data;
      }
      fid_run_free(run);
      free(ff);
    }
}
void EngineFilter::process(CSAMPLE* pInOut, const int iBufferSize)
{
    int i;
    for(i = 0; i < iBufferSize; i += 2)
    {
        pInOut[i + 0] = (CSAMPLE) processSample(fbuf1,  pInOut[i + 0]);
        pInOut[i + 1] = (CSAMPLE) processSample(fbuf2,  pInOut[i + 1]);
    }
}

