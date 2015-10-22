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

_Pragma("once")
#include <memory>
#include <algorithm>

struct FilterConfig{
    unsigned int ord;
    float* ACoeffs;
    float* BCoeffs;
};

class Filter  
{
public:
    Filter( FilterConfig Config );
    virtual ~Filter();
    void reset();
    void process( float *src, float *dst, unsigned int length );
private:
    unsigned int m_ord = 0;
    std::unique_ptr<float[]> m_inBuffer;
    std::unique_ptr<float[]> m_outBuffer;
    float* m_ACoeffs;
    float* m_BCoeffs;
};
