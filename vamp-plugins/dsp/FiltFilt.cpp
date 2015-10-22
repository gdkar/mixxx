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

#include "FiltFilt.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FiltFilt::FiltFilt( FiltFiltConfig Config )
{
    m_ord = Config.ord;
    m_filterConfig.ord = Config.ord;
    m_filterConfig.ACoeffs = Config.ACoeffs;
    m_filterConfig.BCoeffs = Config.BCoeffs;
    m_filter = std::make_unique<Filter>( m_filterConfig );
}
FiltFilt::~FiltFilt() = default;
void FiltFilt::process(float *src, float *dst, unsigned int length)
{	
    unsigned int i;
    if (length == 0) return;
    unsigned int nFilt = m_ord + 1;
    unsigned int nFact = 3 * ( nFilt - 1);
    unsigned int nExt	= length + 2 * nFact;
    m_filtScratchIn .resize( nExt );
    m_filtScratchOut.resize( nExt );
    // Edge transients reflection
    auto sample0 = 2 * src[ 0 ];
    auto sampleN = 2 * src[ length - 1 ];
    auto index = 0;
    for( i = nFact; i > 0; i-- ) m_filtScratchIn[ index++ ] = sample0 - src[ i ];
    index = 0;
    for( i = 0; i < nFact; i++ ) m_filtScratchIn[ (nExt - nFact) + index++ ] = sampleN - src[ (length - 2) - i ];
    index = 0;
    std::copy(&src[0],&src[length],m_filtScratchIn.begin()+nFact);
    ////////////////////////////////
    // Do  0Ph filtering
    m_filter->process( &m_filtScratchIn[0], &m_filtScratchOut[0], nExt);
    std::reverse_copy(m_filtScratchOut.cbegin(),m_filtScratchOut.cend(),m_filtScratchIn.begin());
    // do FILTER again 
    m_filter->process( &m_filtScratchIn[0], &m_filtScratchOut[0], nExt);
    // reverse the series back 
    std::reverse_copy(m_filtScratchOut.cbegin(),m_filtScratchOut.cend(),m_filtScratchIn.begin());
    std::copy(m_filtScratchIn.begin()+nFact,m_filtScratchIn.begin()+(length+nFact),&dst[0]);
}
void FiltFilt::reset()
{
    if(m_filter) m_filter->reset();
}
