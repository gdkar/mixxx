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
#include <numeric>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Filter::Filter( FilterConfig Config )
{
    initialise( Config );
}
Filter::~Filter() = default;
void Filter::initialise( FilterConfig Config )
{
    m_ord = Config.ord;
    m_ACoeffs = Config.ACoeffs;
    m_BCoeffs = Config.BCoeffs;
    m_inBuffer = std::make_unique<double[]>(m_ord+1);
    m_outBuffer = std::make_unique<double[]>(m_ord+1);
    reset();
}
void Filter::deInitialise()
{
    m_inBuffer.reset();
    m_outBuffer.reset();
}
void Filter::reset()
{
    std::fill(&m_inBuffer[0],&m_inBuffer[m_ord+1],0);
    std::fill(&m_outBuffer[0],&m_outBuffer[m_ord+1],0);
}
void Filter::process( double *src, double *dst, unsigned int length )
{
    unsigned int SP,i,j;
    double xin,xout;
    for (SP=0;SP<length;SP++)
    {
        xin=src[SP];
        /* move buffer */
        for ( i = 0; i < m_ord; i++)
            m_inBuffer[ m_ord - i ]=m_inBuffer[ m_ord - i - 1 ];
        m_inBuffer[0]=xin;
        xout =std::inner_product(&m_BCoeffs[0],&m_BCoeffs[m_ord+1],&m_inBuffer[0],0.0)
             -std::inner_product(&m_ACoeffs[0],&m_ACoeffs[m_ord+1],&m_inBuffer[0],0.0);
        dst[ SP ] = xout;
        for ( i = 0; i < m_ord - 1; i++ )
            m_outBuffer[ m_ord - i - 1 ] = m_outBuffer[ m_ord - i - 2 ];
        m_outBuffer[0]=xout;
    } /* end of SP loop */
}
