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
#include <cstring>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Filter::Filter( FilterConfig Config )
    : m_ord(Config.ord)
    , m_outBuffer(new double[m_ord+1])
    , m_inBuffer(new double[m_ord+1])
    , m_ACoeffs (Config.ACoeffs)
    , m_BCoeffs(Config.BCoeffs)
{reset();}
Filter::~Filter(){deInitialise();}
void Filter::initialise( FilterConfig Config ){
    m_ord = Config.ord;
    m_ACoeffs = Config.ACoeffs;
    m_BCoeffs = Config.BCoeffs;
    if(m_inBuffer) delete[] m_inBuffer;
    if(m_outBuffer) delete[] m_outBuffer;
    m_inBuffer = new double[ m_ord + 1 ];
    m_outBuffer = new double[ m_ord + 1 ];
    reset();
}

void Filter::deInitialise(){
    delete[] m_inBuffer;
    delete[] m_outBuffer;
}
void Filter::reset(){
    std::memset(m_inBuffer,0,sizeof(m_inBuffer[0])*m_ord+1);
    std::memset(m_outBuffer,0,sizeof(m_outBuffer[0])*m_ord+1);
}
void Filter::process( double *src, double *dst, unsigned int length ){
    unsigned int SP,i,j;
    double xin,xout;
    for (SP=0;SP<length;SP++){
        xin=src[SP];
        /* move buffer */
        std::memmove(&m_inBuffer[1], &m_inBuffer[0],m_ord*sizeof(m_inBuffer[0]));
        m_inBuffer[0]=xin;
        xout=0.0;
        for (j=0;j< m_ord + 1; j++) xout = xout + m_BCoeffs[ j ] * m_inBuffer[ j ];
        for (j = 0; j < m_ord; j++) xout = xout - m_ACoeffs[ j + 1 ] * m_outBuffer[ j ];
        dst[ SP ] = xout;
        std::memmove(&m_outBuffer[1],&m_outBuffer[0],(m_ord-1)*sizeof(m_outBuffer[0]));
        m_outBuffer[0]=xout;

    } /* end of SP loop */
}



