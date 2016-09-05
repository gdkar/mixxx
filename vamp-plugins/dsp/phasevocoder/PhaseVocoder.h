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

#ifndef PHASEVOCODER_H
#define PHASEVOCODER_H
#include "dsp/transforms/FFT.h"
#include "maths/MathUtilities.h"
#include <math.h>

#include <cassert>

#include <iostream>

template<class T = float>
class PhaseVocoder
{
public:
    PhaseVocoder(int size, int hop);
    virtual ~PhaseVocoder();

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
    void processTimeDomain(const T *src,
                           T *mag, T *phase, T *unwrapped);

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
    void processFrequencyDomain(const T *reals, const T *imags,
                                T *mag, T *phase, T *unwrapped);

    /**
     * Reset the stored phases to zero. Note that this may be
     * necessary occasionally (depending on the application) to avoid
     * loss of floating-point precision in the accumulated unwrapped
     * phase values as they grow.
     */
    void reset();

protected:
    void FFTShift(T *src);
    void getMagnitudes(T *mag);
    void getPhases(T *theta);
    void unwrapPhases(T *theta, T *unwrapped);

    int m_n;
    int m_hop;
    FFTReal *m_fft;
    T *m_time;
    T *m_imag;
    T *m_real;
    T *m_phase;
    T *m_unwrapped;
};
using std::cerr;
using std::endl;

template<class T>
PhaseVocoder<T>::PhaseVocoder(int n, int hop) :
    m_n(n),
    m_hop(hop)
{
    m_fft = new FFTReal(m_n);
    m_time = new T[m_n];
    m_real = new T[m_n];
    m_imag = new T[m_n];
    m_phase = new T[m_n/2 + 1];
    m_unwrapped = new T[m_n/2 + 1];

    for (int i = 0; i < m_n/2 + 1; ++i) {
        m_phase[i] = 0.0;
        m_unwrapped[i] = 0.0;
    }

    reset();
}

template<class T>
PhaseVocoder<T>::~PhaseVocoder()
{
    delete[] m_unwrapped;
    delete[] m_phase;
    delete[] m_real;
    delete[] m_imag;
    delete[] m_time;
    delete m_fft;
}

template<class T>
void PhaseVocoder<T>::FFTShift(T *src)
{
    const int hs = m_n/2;
    for (int i = 0; i < hs; ++i) {
        T tmp = src[i];
        src[i] = src[i + hs];
        src[i + hs] = tmp;
    }
}

template<class T>
void PhaseVocoder<T>::processTimeDomain(const T *src,
                                     T *mag, T *theta,
                                     T *unwrapped)
{
    for (int i = 0; i < m_n; ++i) {
        m_time[i] = src[i];
    }
    FFTShift(m_time);
    m_fft->forward(m_time, m_real, m_imag);
    getMagnitudes(mag);
    getPhases(theta);
    unwrapPhases(theta, unwrapped);
}

template<class T>
void PhaseVocoder<T>::processFrequencyDomain(const T *reals,
                                          const T *imags,
                                          T *mag, T *theta,
                                          T *unwrapped)
{
    for (int i = 0; i < m_n/2 + 1; ++i) {
        m_real[i] = reals[i];
        m_imag[i] = imags[i];
    }
    getMagnitudes(mag);
    getPhases(theta);
    unwrapPhases(theta, unwrapped);
}

template<class T>
void PhaseVocoder<T>::reset()
{
    for (int i = 0; i < m_n/2 + 1; ++i) {
        // m_phase stores the "previous" phase, so set to one step
        // behind so that a signal with initial phase at zero matches
        // the expected values. This is completely unnecessary for any
        // analytical purpose, it's just tidier.
        auto omega = (T(2 * M_PI) * m_hop * i) / m_n;
        m_phase[i] = -omega;
        m_unwrapped[i] = -omega;
    }
}

template<class T>
void PhaseVocoder<T>::getMagnitudes(T *mag)
{
    for (int i = 0; i < m_n/2 + 1; i++) {
	mag[i] = std::hypot(m_real[i] ,m_imag[i] );
    }
}

template<class T>
void PhaseVocoder<T>::getPhases(T *theta)
{
    for (int i = 0; i < m_n/2 + 1; i++) {
	theta[i] = std::atan2(m_imag[i], m_real[i]);
    }
}

template<class T>
void PhaseVocoder<T>::unwrapPhases(T *theta, T *unwrapped)
{
    for (int i = 0; i < m_n/2 + 1; ++i) {

        T omega = (2 * M_PI * m_hop * i) / m_n;
        T expected = m_phase[i] + omega;
        T error = MathUtilities::princarg(theta[i] - expected);

        unwrapped[i] = m_unwrapped[i] + omega + error;

        m_phase[i] = theta[i];
        m_unwrapped[i] = unwrapped[i];
    }
}

#endif
