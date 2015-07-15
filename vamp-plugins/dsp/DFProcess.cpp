/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file 2005-2006 Christian Landone.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "DFProcess.h"
#include "MathUtilities.h"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <alloca.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DFProcess::DFProcess( DFProcConfig Config )
    : filtSrc(nullptr),
      filtDst(nullptr),
      m_filtScratchIn(nullptr),
      m_filtScratchOut(nullptr),
      m_FFOrd(0)
{
    m_length = Config.length;
    m_winPre = Config.winPre;
    m_winPost = Config.winPost;
    m_alphaNormParam = Config.AlphaNormParam;

    m_isMedianPositive = Config.isMedianPositive;

    filtSrc = new float[ m_length ];
    filtDst = new float[ m_length ];
	
    //Low Pass Smoothing Filter Config
    m_FilterConfigParams.ord = Config.LPOrd;
    m_FilterConfigParams.ACoeffs = Config.LPACoeffs;
    m_FilterConfigParams.BCoeffs = Config.LPBCoeffs;
	
    m_FiltFilt = new FiltFilt( m_FilterConfigParams );	
}

DFProcess::~DFProcess()
{
    delete [] filtSrc;
    delete [] filtDst;
    delete [] m_filtScratchIn;
    delete [] m_filtScratchOut;
    delete m_FiltFilt;
}
void DFProcess::process(float *src, float* dst)
{
    if (m_length == 0) return;
    removeDCNormalize( src, filtSrc );
    m_FiltFilt->process( filtSrc, filtDst, m_length );
    medianFilter( filtDst, dst );
}
void DFProcess::medianFilter(float *src, float *dst)
{
    int i,k,j,l;
    int index = 0;
    float val = 0;
    auto y = (float*)alloca(sizeof(float)*(m_winPost+m_winPre+1));
    auto scratch = (float*)alloca(sizeof(float)*(m_winPost+m_winPre+1));
    memset( y, 0, sizeof( float ) * ( m_winPost + m_winPre + 1) );
    for( i = 0; i < m_winPre; i++,index++){
        if (index >= m_length) break;
	k = i + m_winPost + 1;
	for( j = 0; j < k; j++){y[ j ] = src[ j ];}
	scratch[ index ] = MathUtilities::median( y, k );
    }
    for(  i = 0; i + m_winPost + m_winPre < m_length; i ++){
        if (index >= m_length) break;
	for(l=0,  j  = i; j < ( i + m_winPost + m_winPre + 1); j++,l++){
	    y[ l ] = src[ j ];
	}
	scratch[ index++ ] = MathUtilities::median( y, (m_winPost + m_winPre + 1 ));
    }
    for( i = std::max( m_length - m_winPost, 1); i < m_length; i++)
    {
        if (index >= m_length) break;
	k = std::max( i - m_winPre, 1);
	for(l=0, j = k; j < m_length; j++,l++){
	    y[ l ] = src[ j ];
	}
	scratch[ index++ ] = MathUtilities::median( y, l); 
    }
    for( i = 0; i < m_length; i++ ){
	val = src[ i ] - scratch[ i ];
	if( m_isMedianPositive ){dst[i]=std::max(val,0.f);}
	else{dst[ i ]  = val;}
    }
}
void DFProcess::removeDCNormalize( float *src, float*dst )
{
    float DFmax = 0;
    float DFMin = 0;
    float DFAlphaNorm = 0;
    MathUtilities::getFrameMinMax( src, m_length, &DFMin, &DFmax );
    MathUtilities::getAlphaNorm( src, m_length, m_alphaNormParam, &DFAlphaNorm );
    DFAlphaNorm = 1.f/DFAlphaNorm;
    for( unsigned int i = 0; i< m_length; i++){dst[ i ] = ( src[ i ] - DFMin ) * DFAlphaNorm; }
}
