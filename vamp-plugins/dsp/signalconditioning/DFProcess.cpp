/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file 2005-2006 Christian Landone.

    Modifications:

    - delta threshold
    Description: add delta threshold used as offset in the smoothed
    detection function
    Author: Mathieu Barthet
    Date: June 2010

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "DFProcess.h"
#include "maths/MathUtilities.h"

#include <cstring>
#include <algorithm>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DFProcess::DFProcess( DFProcConfig Config )
{
    filtSrc = NULL;
    filtDst = NULL;	
    m_filtScratchIn = NULL;
    m_filtScratchOut = NULL;
    m_FFOrd = 0;
    initialise( Config );
}
DFProcess::~DFProcess()
{
    deInitialise();
}

void DFProcess::initialise( DFProcConfig Config )
{
    m_length = Config.length;
    m_winPre = Config.winPre;
    m_winPost = Config.winPost;
    m_alphaNormParam = Config.AlphaNormParam;

    m_isMedianPositive = Config.isMedianPositive;

    filtSrc = new double[ m_length ];
    filtDst = new double[ m_length ];

	
    //Low Pass Smoothing Filter Config
    m_FilterConfigParams.ord = Config.LPOrd;
    m_FilterConfigParams.ACoeffs = Config.LPACoeffs;
    m_FilterConfigParams.BCoeffs = Config.LPBCoeffs;
	
    m_FiltFilt = new FiltFilt( m_FilterConfigParams );
	
    //add delta threshold
    m_delta = Config.delta;
}

void DFProcess::deInitialise()
{
    delete [] filtSrc;
    delete [] filtDst;
    delete [] m_filtScratchIn;
    delete [] m_filtScratchOut;
    delete m_FiltFilt;
}

void DFProcess::process(double *src, double* dst)
{
    if (m_length == 0) return;
    removeDCNormalize( src, filtSrc );
    m_FiltFilt->process( filtSrc, filtDst, m_length );
    medianFilter( filtDst, dst );
}
void DFProcess::medianFilter(double *src, double *dst)
{
    for(auto i = 0; i < m_length; i++) {
        dst[i] = MathUtilities::median(src+std::max(0, i - m_winPre),src+std::min(m_length, i + m_winPost + 1));
    }
    if(m_isMedianPositive) {
        std::transform(src,src+m_length,dst,dst,[this](auto s,auto d){return std::max(s - d - m_delta, 0.);});
    }else{
        std::transform(src,src+m_length,dst,dst, [this](auto s,auto d){return s - d - m_delta;});
    }
}
void DFProcess::removeDCNormalize( double *src, double*dst )
{
    auto DFMin = *std::min_element(src,std::next(src,m_length));
    auto DFAlphaNorm = 1. / MathUtilities::getAlphaNorm( src, std::next(src,m_length), m_alphaNormParam);
    std::transform(src,std::next(src,m_length),dst,
            [=](auto x){return (x - DFMin) * DFAlphaNorm;});
}
