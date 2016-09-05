/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
*/

#include "FFT.h"

#include "maths/MathUtilities.h"

#include "ext/kissfft/kiss_fft.h"
#include "ext/kissfft/kiss_fftr.h"

#include <cmath>

#include <iostream>

#include <stdexcept>

FFT::FFT(int n) :
    m_d(new D(n))
{
}

FFT::~FFT()
{
    delete m_d;
}
FFTReal::FFTReal(int n) :
    m_d(new D(n))
{
}
FFTReal::~FFTReal()
{
    delete m_d;
}
