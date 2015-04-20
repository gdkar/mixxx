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

#ifndef FILTFILT_H
#define FILTFILT_H

#include "Filter.h"

struct FiltFiltConfig{
    unsigned int ord;
    double* ACoeffs;
    double* BCoeffs;
};

class FiltFilt  
{
public:
    FiltFilt( FiltFiltConfig Config );
    virtual ~FiltFilt();
    void reset();
    void process( double* src, double* dst, unsigned int length );
private:
    void initialise( FiltFiltConfig Config );
    void deInitialise();
    unsigned int m_ord;
    Filter* m_filter;
    double* m_filtScratchIn;
    double* m_filtScratchOut;
    FilterConfig m_filterConfig;
};

#endif
