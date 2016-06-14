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
_Pragma("once")
#include <memory>
#include <utility>
#include <functional>
#include <algorithm>
#include <iterator>
#include <type_traits>
#include <typeinfo>
#include <cassert>
#include <cstdio>
#include <iostream>

#include "dsp/transforms/FFT.h"
#include "maths/MathUtilities.h"
template<typename T>
class PhaseVocoder  
{
public:
    PhaseVocoder(int n, int hop)
    : m_n(n)
    , m_hop(hop)
    , m_fft(std::make_unique<FFTReal>(n))
    , m_time(std::make_unique<T[]>(n))
    , m_real(std::make_unique<T[]>(n))
    , m_imag(std::make_unique<T[]>(n))
    , m_phase(std::make_unique<T[]>(n))
    , m_unwrapped(std::make_unique<T[]>(n))
    {
        reset();
    }
    virtual ~PhaseVocoder() = default;
    /**
     * Given one frame of time-domain samples, FFT and return the
     * magnitudes, instantaneous phases, and unwrapped phases.
     *
     * src must have size values (where size is the frame size value
     * as passed to the PhaseVocoder constructor), and should have
     * been windowed as necessary by the caller (but not fft-shifted).
     *
     * mag, phase, and unwrapped must each be non-NULL and point to
     * enough space for size/2 + 1 values. The redundant conjugate
     * half of the output is not returned.
     */
    template<typename U>
    void processTimeDomain(const U *src,
                                        U  *mag, U *theta,
                                        U *unwrapped)
    {
        std::copy(&src[0],&src[m_n],&m_time[0]);
        FFTShift(m_time.get());
        m_fft->forward(m_time.get(), m_real.get(), m_imag.get());
        getMagnitudes(mag);
        getPhases(theta);
        unwrapPhases(theta, unwrapped);
    }
    /**
     * Given one frame of frequency-domain samples, return the
     * magnitudes, instantaneous phases, and unwrapped phases.
     *
     * reals and imags must each contain size/2+1 values (where size
     * is the frame size value as passed to the PhaseVocoder
     * constructor).
     *
     * mag, phase, and unwrapped must each be non-NULL and point to
     * enough space for size/2+1 values.
     */
    template<typename U>
    void processFrequencyDomain(const U *reals, 
                                const U *imags,
                                U *mag,
                                U *theta,
                                U *unwrapped)
    {
        std::copy(&reals[0],&reals[m_n/2+1],&m_real[0]);
        std::copy(&imags[0],&imags[m_n/2+1],&m_imag[0]);
        getMagnitudes(mag);
        getPhases(theta);
        unwrapPhases(theta, unwrapped);
    }
    /**
     * Reset the stored phases to zero. Note that this may be
     * necessary occasionally (depending on the application) to avoid
     * loss of floating-point precision in the accumulated unwrapped
     * phase values as they grow.
     */
    void reset()
    {
        auto i = 0;
        auto inc = T(2*M_PI) * m_hop  / m_n;
        std::generate(&m_phase[0],&m_phase[m_n/2+1],[&](){ return i += inc;});
        std::copy(&m_phase[0],&m_phase[m_n/2+1],&m_unwrapped[i]);
    }
protected:
    template<typename U>
    void FFTShift(U *src) { std::rotate(&src[0],&src[m_n/2],&src[m_n]); }

    template<typename U>
    void getMagnitudes(U *mag)
    {   
        std::transform(&m_imag[0],&m_imag[m_n/2+1],&m_real[0],&mag[0],[](auto x, auto y){return std::hypot(x,y);});
    }
    template<typename U>
    void getPhases(U *theta)
    {
        std::transform(&m_imag[0],&m_imag[m_n/2+1],&m_real[0],&theta[0],[](auto x, auto y){return std::atan2(x,y);});
    }
    template<typename U>
    void unwrapPhases(U *theta, U *unwrapped)
    {
        for (int i = 0; i < m_n/2 + 1; ++i) {
            auto omega = (T(2 * M_PI) * m_hop * i) / m_n;
            auto expected = m_phase[i] + omega;
            auto error = MathUtilities::princarg(theta[i] - expected);
            unwrapped[i] = m_unwrapped[i] + omega + error;
            m_phase[i] = theta[i];
            m_unwrapped[i] = unwrapped[i];
        }
    }
    int m_n;
    int m_hop;
    std::unique_ptr<FFTReal > m_fft;
    std::unique_ptr<T[]>     m_time;
    std::unique_ptr<T[]>m_imag;
    std::unique_ptr<T[]>m_real;
    std::unique_ptr<T[]>m_phase;
    std::unique_ptr<T[]>m_unwrapped;

};
