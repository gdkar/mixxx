/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file 2005-2006 Christian Landone, copyright 2013 QMUL.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <algorithm>
#include "PhaseVocoder.h"
#include "dsp/transforms/FFT.h"
#include "maths/MathUtilities.h"
#include <cmath>

#include <cassert>

#include <iostream>
using std::cerr;
using std::endl;

PhaseVocoder::PhaseVocoder(int n, int hop) :
    m_n(n),
    m_hop(hop)
{
    reset();
}

PhaseVocoder::~PhaseVocoder() = default;

void PhaseVocoder::FFTShift(float *src)
{
    std::rotate(src,src + m_n/2, src + m_n);
}

void PhaseVocoder::processTimeDomain(const float *src,
                                     float *mag, float *theta,
                                     float *unwrapped)
{
    std::copy_n(src,m_n,std::begin(m_time));
    FFTShift(&m_time[0]);
    m_fft.forward(&m_time[0], m_real.data(), m_imag.data());
    getMagnitudes(mag);
    getPhases(theta);
    unwrapPhases(theta, unwrapped);
}

void PhaseVocoder::processFrequencyDomain(const float *reals,
                                          const float *imags,
                                          float *mag, float *theta,
                                          float *unwrapped)
{
    std::copy_n(reals,m_n/2+1,std::begin(m_real));
    std::copy_n(imags,m_n/2+1,std::begin(m_imag));
    getMagnitudes(mag);
    getPhases(theta);
    unwrapPhases(theta, unwrapped);
}

void PhaseVocoder::reset()
{
    if(m_n) {
        auto omegaFactor = float(2 * M_PI * m_hop) / m_n;
        for (int i = 0; i < m_n/2 + 1; ++i) {
            // m_phase stores the "previous" phase, so set to one step
            // behind so that a signal with initial phase at zero matches
            // the expected values. This is completely unnecessary for any
            // analytical purpose, it's just tidier.
            auto omega = omegaFactor * i;
            m_phase[i] = -omega;
            m_unwrapped[i] = -omega;
        }
    }
}

void PhaseVocoder::getMagnitudes(float *_mag)
{
    std::transform(std::begin(m_imag),std::begin(m_imag) + m_n/2+1,std::begin(m_real),
        _mag,[](auto &&x,auto &&y){return std::hypot(x,y);});
}

void PhaseVocoder::getPhases(float *t)
{
    std::transform(std::begin(m_imag),std::begin(m_imag)+m_n/2+1,std::begin(m_real),
        t,[](auto &&x, auto &&y){return std::atan2(x,y);});
}

void PhaseVocoder::unwrapPhases(float *theta, float *unwrapped)
{
    auto omegaFactor = float(2 * M_PI * m_hop) / m_n;
    for (int i = 0; i < m_n/2 + 1; ++i) {

        auto omega    = i * omegaFactor;
        auto expected = m_phase[i] + omega;
        auto error    = MathUtilities::princarg(theta[i] - expected);

        unwrapped[i]   = m_unwrapped[i] + omega + error;

        m_phase[i]     = theta[i];
        m_unwrapped[i] = unwrapped[i];
    }
}
