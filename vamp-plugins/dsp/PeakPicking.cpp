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

#include "PeakPicking.h"
#include "Polyfit.h"

#include <iostream>
#include <cstring>
#include <utility>
#include <iterator>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
PeakPicking::PeakPicking( PPickParams Config )
    :m_workBuffer(std::make_unique<float[]>(Config.length))
{
    m_DFLength = Config.length ;
    Qfilta = Config.QuadThresh.a ;
    Qfiltb = Config.QuadThresh.b ;
    Qfiltc = Config.QuadThresh.c ;
	
    m_DFProcessingParams.length = m_DFLength; 
    m_DFProcessingParams.LPOrd = Config.LPOrd; 
    m_DFProcessingParams.LPACoeffs = Config.LPACoeffs; 
    m_DFProcessingParams.LPBCoeffs = Config.LPBCoeffs; 
    m_DFProcessingParams.winPre  = Config.WinT.pre;
    m_DFProcessingParams.winPost = Config.WinT.post; 
    m_DFProcessingParams.AlphaNormParam = Config.alpha;
    m_DFProcessingParams.isMedianPositive = false;
	
    m_DFSmoothing = std::make_unique<DFProcess>( m_DFProcessingParams );
    std::fill(&m_workBuffer[0],&m_workBuffer[m_DFLength],0);
}
PeakPicking::~PeakPicking() = default;
void PeakPicking::process( float* src, unsigned int len, vector<int> &onsets )
{
    if (len < 4) return;
    vector <float> m_maxima;	
    // Signal conditioning 
    m_DFSmoothing->process( src, &m_workBuffer[0] );
    std::move(&m_workBuffer[0],&m_workBuffer[len],std::back_inserter(m_maxima));
    quadEval( m_maxima, onsets );
    std::move(m_maxima.cbegin(),m_maxima.cend(),&src[0]);
}

int PeakPicking::quadEval( vector<float> &src, vector<int> &idx )
{
    vector <int> m_maxIndex;
    vector <int> m_onsetPosition;
    vector <float> m_maxFit;
    vector <float> m_poly;
    vector <float> m_err;
    auto maxLength = decltype(m_maxIndex.size()){0};
    auto p = 0.f;
    m_poly.push_back(0);
    m_poly.push_back(0);
    m_poly.push_back(0);
    for(  auto t = -2; t < 3; t++)
    {
	m_err.push_back( (float)t );
    }
    for( auto i = decltype(src.size()){2}; i < src.size() - 2; i++)
    {
	if( (src[i] > src[i-1]) && (src[i] > src[i+1]) && (src[i] > 0) )
	{
//	    m_maxIndex.push_back(  i + 1 );
            m_maxIndex.push_back(i);
	}
    }
    maxLength = m_maxIndex.size();
    auto selMax = 0.f;
    for( auto j = decltype(maxLength){0}; j < maxLength ; j++)
    {
        for (auto k = -2; k <= 2; ++k)
	{
	    selMax = src[ m_maxIndex[j] + k ] ;
	    m_maxFit.push_back(selMax);			
	}
	p = TPolyFit::PolyFit2( m_err, m_maxFit, m_poly);
	auto f = m_poly[0];
	auto g = m_poly[1];
	auto h = m_poly[2];

	int kk = m_poly.size();

	if (h < -Qfilta || f > Qfiltc) idx.push_back(m_maxIndex[j]);
		
	m_maxFit.clear();
    }

    return 1;
}
