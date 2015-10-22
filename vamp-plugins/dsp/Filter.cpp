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

#include "Filter.h"
#include <algorithm>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Filter::Filter( FilterConfig Config )
{
    m_ord = Config.ord;
    m_ACoeffs = Config.ACoeffs;
    m_BCoeffs = Config.BCoeffs;
    m_inBuffer = std::make_unique<float[]>( m_ord + 1 );
    m_outBuffer = std::make_unique<float[]>( m_ord + 1 );
    reset();
}
Filter::~Filter() = default;
void Filter::reset()
{
    std::fill_n(&m_inBuffer[0],0,m_ord+1);
    std::fill_n(&m_outBuffer[0],0,m_ord+1);
}
void Filter::process( float *src, float *dst, unsigned int length )
{
    unsigned int SP,i,j;
    float xin;
    for (SP=0;SP<length;SP++)
    {
        xin=src[SP];
        /* move buffer */
        std::copy_backward(&m_inBuffer[0],&m_inBuffer[m_ord+1],&m_inBuffer[1]);
        m_inBuffer[0]=xin;
        auto xout= std::inner_product( &m_BCoeffs[0],&m_BCoeffs[m_ord+1], &m_inBuffer[0],0.f);
        xout    -= std::inner_product( &m_ACoeffs[1],&m_ACoeffs[m_ord+1], &m_inBuffer[0],0.f);
        for (j=0;j< m_ord + 1; j++) xout = xout + m_BCoeffs[ j ] * m_inBuffer[ j ];
        for (j = 0; j < m_ord; j++) xout = xout - m_ACoeffs[ j + 1 ] * m_outBuffer[ j ];
        dst[ SP ] = xout;
        std::copy_backward(&m_outBuffer[1],&m_outBuffer[m_ord+1],&m_outBuffer[0]);
        m_outBuffer[0]=xout;
    } /* end of SP loop */
}



