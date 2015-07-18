////////////////////////////////////////////////////////////////////////////////
///
/// FIR low-pass (anti-alias) filter with filter coefficient design routine and
/// MMX optimization. 
/// 
/// Anti-alias filter is used to prevent folding of high frequencies when 
/// transposing the sample rate with interpolation.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2014-01-05 16:40:22 -0500 (Sun, 05 Jan 2014) $
// File revision : $Revision: 4 $
//
// $Id: AAFilter.cpp 177 2014-01-05 21:40:22Z oparviai $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include <memory>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include "AAFilter.h"
#include "FIRFilter.h"

using namespace soundtouch;
static const double TWOPI = 2*M_PI;
// define this to save AA filter coefficients to a file
// #define _DEBUG_SAVE_AAFILTER_COEFFICIENTS   1
#define _DEBUG_SAVE_AAFIR_COEFFS(x, y)


/*****************************************************************************
 *
 * Implementation of the class 'AAFilter'
 *
 *****************************************************************************/

AAFilter::AAFilter(uint len)
  : pFIR(FIRFilter::newInstance())
  , cutoffFreq(0.5)
  , length(len)
{
  calculateCoeffs();
}
AAFilter::~AAFilter()
{
    delete pFIR;
}



// Sets new anti-alias filter cut-off edge frequency, scaled to
// sampling frequency (nyquist frequency = 0.5).
// The filter will cut frequencies higher than the given frequency.
void AAFilter::setCutoffFreq(double newCutoffFreq){
    cutoffFreq = newCutoffFreq;
    calculateCoeffs();
}
// Sets number of FIR filter taps
void AAFilter::setLength(uint newLength){
    length = newLength;
    calculateCoeffs();
}

// Calculates coefficients for a low-pass FIR filter using Hamming window
void AAFilter::calculateCoeffs()
{
    uint i;
    double cntTemp, temp, tempCoeff,h, w;
    double wc;
    double scaleCoeff, sum;
    double *work;
    CSAMPLE *coeffs;

    assert(length >= 2);
    assert(length % 4 == 0);
    assert(cutoffFreq >= 0);
    assert(cutoffFreq <= 0.5);

    work =   new double[length];
    coeffs = new CSAMPLE[length];

    wc = TWOPI * cutoffFreq;
    tempCoeff = TWOPI / (double)length;

    sum = 0;
    for (i = 0; i < length; i ++) 
    {
        cntTemp = (double)i - (double)(length / 2);

        temp = cntTemp * wc;
        if (temp != 0) 
        {
            h = sin(temp) / temp;                     // sinc function
        } 
        else 
        {
            h = 1.0;
        }
        w = 0.54 + 0.46 * cos(tempCoeff * cntTemp);       // hamming window

        temp = w * h;
        work[i] = temp;

        // calc net sum of coefficients 
        sum += temp;
    }

    // ensure the sum of coefficients is larger than zero
    assert(sum > 0);

    // ensure we've really designed a lowpass filter...
    assert(work[length/2] > 0);
    assert(work[length/2 + 1] > -1e-6);
    assert(work[length/2 - 1] > -1e-6);

    // Calculate a scaling coefficient in such a way that the result can be
    // divided by 16384
    scaleCoeff = 16384.0f / sum;

    for (i = 0; i < length; i ++) 
    {
        temp = work[i] * scaleCoeff;
//#if SOUNDTOUCH_INTEGER_SAMPLES
        // scale & round to nearest integer
        temp += (temp >= 0) ? 0.5 : -0.5;
        // ensure no overfloods
        assert(temp >= -32768 && temp <= 32767);
//#endif
        coeffs[i] = (CSAMPLE)temp;
    }

    // Set coefficients. Use divide factor 14 => divide result by 2^14 = 16384
    pFIR->setCoefficients(coeffs, length, 14);

    _DEBUG_SAVE_AAFIR_COEFFS(coeffs, length);

    delete[] work;
    delete[] coeffs;
}


// Applies the filter to the given sequence of samples. 
// Note : The amount of outputted samples is by value of 'filter length' 
// smaller than the amount of input samples.
uint AAFilter::evaluate(CSAMPLE *dest, const CSAMPLE *src, uint size, uint numChannels) const
{
    return pFIR->evaluate(dest, src, size, numChannels);
}


/// Applies the filter to the given src & dest pipes, so that processed amount of
/// samples get removed from src, and produced amount added to dest 
/// Note : The amount of outputted samples is by value of 'filter length' 
/// smaller than the amount of input samples.
uint AAFilter::evaluate(FIFOSampleBuffer &dest, FIFOSampleBuffer &src) const
{
    CSAMPLE *pdest;
    const CSAMPLE *psrc;
    uint numSrcSamples;
    uint result;
    int numChannels = src.getChannels();

    assert(numChannels == dest.getChannels());

    numSrcSamples = src.size();
    psrc = src.begin();
    pdest = dest.end(numSrcSamples);
    result = pFIR->evaluate(pdest, psrc, numSrcSamples, numChannels);
    src.receiveSamples(result);
    dest.putSamples(result);

    return result;
}


uint AAFilter::getLength() const
{
    return pFIR->getLength();
}
