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

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DFProcess::DFProcess( DFProcConfig Config )
{
    m_FFOrd = 0;
    m_length = Config.length;
    m_winPre = Config.winPre;
    m_winPost = Config.winPost;
    m_alphaNormParam = Config.AlphaNormParam;

    m_isMedianPositive = Config.isMedianPositive;

    filtSrc = std::make_unique<float[]>( m_length );
    filtDst = std::make_unique<float[]>( m_length );

	
    //Low Pass Smoothing Filter Config
    m_FilterConfigParams.ord = Config.LPOrd;
    m_FilterConfigParams.ACoeffs = Config.LPACoeffs;
    m_FilterConfigParams.BCoeffs = Config.LPBCoeffs;
	
    m_FiltFilt = std::make_unique<FiltFilt>( m_FilterConfigParams );	
}
DFProcess::~DFProcess() = default;
void DFProcess::process(float *src, float* dst)
{
    if (m_length == 0) return;
    removeDCNormalize( src, &filtSrc[0] );
    m_FiltFilt->process( &filtSrc[0], &filtDst[0], m_length );
    medianFilter( &filtDst[0], dst );
}
void DFProcess::medianFilter(float *src, float *dst)
{
    auto  j = 0,l = 0;
    auto index = 0;
    auto y  = std::make_unique<float[]>(m_winPost + m_winPre + 1);
    auto scratch = std::make_unique<float[]>( m_length );
    for( auto i = 0; i < m_winPre; i++)
    {
        if (index >= m_length) break;
	auto k = i + m_winPost + 1;
	for( j = 0; j < k; j++)
	{
	    y[ j ] = src[ j ];
	}
	scratch[ index ] = MathUtilities::median( &y[0], k );
	index++;
    }
    for(  auto i = 0; i + m_winPost + m_winPre < m_length; i ++)
    {
        if (index >= m_length) break;
	l = 0;
	for(  j  = i; j < ( i + m_winPost + m_winPre + 1); j++)
	{
	    y[ l ] = src[ j ];
	    l++;
	}
	scratch[ index++ ] = MathUtilities::median( &y[0], (m_winPost + m_winPre + 1 ));
    }
    for( auto i = std::max( m_length - m_winPost, 1); i < m_length; i++)
    {
        if (index >= m_length) break;
	auto k = std::max( i - m_winPre, 1);
	l = 0;
	for( j = k; j < m_length; j++)
	{
	    y[ l ] = src[ j ];

	    l++;
	}
	scratch[ index++ ] = MathUtilities::median( &y[0], l); 
    }
    for( auto i = 0; i < m_length; i++ )
    {
	auto val = src[ i ] - scratch[ i ];// - 0.033;
	if( m_isMedianPositive )
	{
            dst[ i ] = std::max(val,0.f);
	}
	else dst[ i ]  = val;
    }
}
void DFProcess::removeDCNormalize( float *src, float*dst )
{
    auto DFmax = 0.f;
    auto DFMin = 0.f;
    auto DFAlphaNorm = 0.f;
    MathUtilities::getFrameMinMax( src, m_length, &DFMin, &DFmax );
    MathUtilities::getAlphaNorm( src, m_length, m_alphaNormParam, &DFAlphaNorm );
    DFAlphaNorm = 1.f/DFAlphaNorm;
    for( unsigned int i = 0; i< m_length; i++) dst[ i ] = ( src[ i ] - DFMin ) * DFAlphaNorm; 
}
