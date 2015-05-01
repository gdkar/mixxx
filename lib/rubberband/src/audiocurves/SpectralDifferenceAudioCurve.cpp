/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2014 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#include "SpectralDifferenceAudioCurve.h"

#include "system/Allocators.h"
#include "system/VectorOps.h"
#include <cmath>
namespace RubberBand
{


SpectralDifferenceAudioCurve::SpectralDifferenceAudioCurve(Parameters parameters) :
    AudioCurveCalculator(parameters)
{
    m_mag = allocate<double>(m_lastPerceivedBin + 1);
    v_zero(m_mag, m_lastPerceivedBin + 1);
}

SpectralDifferenceAudioCurve::~SpectralDifferenceAudioCurve()
{
    deallocate(m_mag);
}

void
SpectralDifferenceAudioCurve::reset()
{
    v_zero(m_mag, m_lastPerceivedBin + 1);
}

void
SpectralDifferenceAudioCurve::setFftSize(int newSize){
    deallocate(m_tmpbuf);
    deallocate(m_mag);
    AudioCurveCalculator::setFftSize(newSize);
    m_mag = allocate<double>(m_lastPerceivedBin + 1);
    reset();
}

float
SpectralDifferenceAudioCurve::processFloat(const float *R__ mag, int increment)
{
    float result = 0.0;

    const int hs1 = m_lastPerceivedBin + 1;
    for(int i = 0; i < hs1; i++){
        const float conv = (float)mag[i];
        const float sqr  = conv*conv;
        const float sqr_minus = sqr-m_mag[i];
        const float sqrt_abs  = sqrt(sqr_minus<0?-sqr_minus:sqr_minus);
        result +=  sqrt_abs;
        m_mag[i] = sqr;

    }
    return result;

}

double
SpectralDifferenceAudioCurve::processDouble(const double *R__ mag, int increment)
{
    double result = 0.0;

    const int hs1 = m_lastPerceivedBin + 1;
    for(int i = 0; i < hs1; i++){
        const double conv = (double)mag[i];
        const double sqr  = conv*conv;
        const double sqr_minus = sqr-m_mag[i];
        const double sqrt_abs  = sqrt(sqr_minus<0?-sqr_minus:sqr_minus);
        result +=  sqrt_abs;
        m_mag[i] = sqr;

    }
    return result;
}

}

