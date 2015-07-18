////////////////////////////////////////////////////////////////////////////////
///
/// General FIR digital filter routines with MMX optimization. 
///
/// Note : MMX optimized functions reside in a separate, platform-specific file, 
/// e.g. 'mmx_win.cpp' or 'mmx_gcc.cpp'
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2014-10-08 11:26:57 -0400 (Wed, 08 Oct 2014) $
// File revision : $Revision: 4 $
//
// $Id: FIRFilter.cpp 201 2014-10-08 15:26:57Z oparviai $
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

#include <memory.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "FIRFilter.h"
#include "cpu_detect.h"

using namespace soundtouch;

/*****************************************************************************
 *
 * Implementation of the class 'FIRFilter'
 *
 *****************************************************************************/

FIRFilter::FIRFilter()
{
    resultDivFactor = 0;
    resultDivider = 0;
    length = 0;
    lengthDiv8 = 0;
    filterCoeffs = NULL;
    sum = NULL;
    sumsize = 0;
}


FIRFilter::~FIRFilter()
{
    delete[] filterCoeffs;
    delete[] sum;
}

// Usual C-version of the filter routine for stereo sound
uint FIRFilter::evaluateFilterStereo(CSAMPLE *dest, const CSAMPLE *src, uint size) const
{
    uint i, j, end;
    CSAMPLE suml, sumr;
#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    // when using floating point samples, use a scaler instead of a divider
    // because division is much slower operation than multiplying.
    double dScaler = 1.0 / (double)resultDivider;
#endif

    assert(length != 0);
    assert(src != NULL);
    assert(dest != NULL);
    assert(filterCoeffs != NULL);

    end = 2 * (size - length);

    for (j = 0; j < end; j += 2) 
    {
        const CSAMPLE *ptr;

        suml = sumr = 0;
        ptr = src + j;

        for (i = 0; i < length; i += 4) 
        {
            // loop is unrolled by factor of 4 here for efficiency
            suml += ptr[2 * i + 0] * filterCoeffs[i + 0] +
                    ptr[2 * i + 2] * filterCoeffs[i + 1] +
                    ptr[2 * i + 4] * filterCoeffs[i + 2] +
                    ptr[2 * i + 6] * filterCoeffs[i + 3];
            sumr += ptr[2 * i + 1] * filterCoeffs[i + 0] +
                    ptr[2 * i + 3] * filterCoeffs[i + 1] +
                    ptr[2 * i + 5] * filterCoeffs[i + 2] +
                    ptr[2 * i + 7] * filterCoeffs[i + 3];
        }

#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        suml >>= resultDivFactor;
        sumr >>= resultDivFactor;
        // saturate to 16 bit integer limits
        suml = (suml < -32768) ? -32768 : (suml > 32767) ? 32767 : suml;
        // saturate to 16 bit integer limits
        sumr = (sumr < -32768) ? -32768 : (sumr > 32767) ? 32767 : sumr;
#else
        suml *= dScaler;
        sumr *= dScaler;
#endif // SOUNDTOUCH_INTEGER_SAMPLES
        dest[j] = (CSAMPLE)suml;
        dest[j + 1] = (CSAMPLE)sumr;
    }
    return size - length;
}




// Usual C-version of the filter routine for mono sound
uint FIRFilter::evaluateFilterMono(CSAMPLE *dest, const CSAMPLE *src, uint size) const
{
    uint i, j, end;
    CSAMPLE sum;
#ifdef SOUNDTOUCH_FLOAT_SAMPLES
    // when using floating point samples, use a scaler instead of a divider
    // because division is much slower operation than multiplying.
    double dScaler = 1.0 / (double)resultDivider;
#endif


    assert(length != 0);

    end = size - length;
    for (j = 0; j < end; j ++) 
    {
        sum = 0;
        for (i = 0; i < length; i += 4) 
        {
            // loop is unrolled by factor of 4 here for efficiency
            sum += src[i + 0] * filterCoeffs[i + 0] + 
                   src[i + 1] * filterCoeffs[i + 1] + 
                   src[i + 2] * filterCoeffs[i + 2] + 
                   src[i + 3] * filterCoeffs[i + 3];
        }
#ifdef SOUNDTOUCH_INTEGER_SAMPLES
        sum >>= resultDivFactor;
        // saturate to 16 bit integer limits
        sum = (sum < -32768) ? -32768 : (sum > 32767) ? 32767 : sum;
#else
        sum *= dScaler;
#endif // SOUNDTOUCH_INTEGER_SAMPLES
        dest[j] = (CSAMPLE)sum;
        src ++;
    }
    return end;
}


uint FIRFilter::evaluateFilterMulti(CSAMPLE *dest, const CSAMPLE *src, uint size, uint numChannels)
{
    uint i, j, end, c;

    if (sumsize < numChannels){
        // allocate large enough array for keeping sums
        sumsize = numChannels;
        delete[] sum;
        sum = new CSAMPLE[numChannels];
    }
    // when using floating point samples, use a scaler instead of a divider
    // because division is much slower operation than multiplying.
    CSAMPLE dScaler = 1.0f / (CSAMPLE)resultDivider;
    assert(length != 0);
    assert(src != NULL);
    assert(dest != NULL);
    assert(filterCoeffs != NULL);
    end = numChannels * (size - length);
    for (c = 0; c < numChannels; c ++){sum[c] = 0;}
    for (j = 0; j < end; j += numChannels)
    {
        const CSAMPLE *ptr;
        ptr = src + j;
        for (i = 0; i < length; i ++)
        {
            CSAMPLE coef=filterCoeffs[i];
            for (c = 0; c < numChannels; c ++)
            {
                sum[c] += ptr[0] * coef;
                ptr ++;
            }
        }
        for (c = 0; c < numChannels; c ++)
        {
            sum[c] *= dScaler;
            *dest = (CSAMPLE)sum[c];
            dest++;
            sum[c] = 0;
        }
    }
    return size - length;
}


// Set filter coeffiecients and length.
//
// Throws an exception if filter length isn't divisible by 8
void FIRFilter::setCoefficients(const CSAMPLE *coeffs, uint newLength, uint uResultDivFactor)
{
    assert(newLength > 0);
    if (newLength % 8) ST_THROW_RT_ERROR("FIR filter length not divisible by 8");

    lengthDiv8 = newLength / 8;
    length = lengthDiv8 * 8;
    assert(length == newLength);

    resultDivFactor = uResultDivFactor;
    resultDivider = (CSAMPLE)(1<<(int)resultDivFactor);
    delete[] filterCoeffs;
    filterCoeffs = reinterpret_cast<CSAMPLE*>(new long double[length*sizeof(CSAMPLE)/sizeof(long double)]);
    memcpy(filterCoeffs, coeffs, length * sizeof(CSAMPLE));
}
uint FIRFilter::getLength() const{return length;}
// Applies the filter to the given sequence of samples. 
//
// Note : The amount of outputted samples is by value of 'filter_length' 
// smaller than the amount of input samples.
uint FIRFilter::evaluate(CSAMPLE *dest, const CSAMPLE *src, uint size, uint numChannels) 
{
    assert(length > 0);
    assert(lengthDiv8 * 8 == length);

    if (size < length) return 0;

#ifndef USE_MULTICH_ALWAYS
    if (numChannels == 1)
    {
        return evaluateFilterMono(dest, src, size);
    } 
    else if (numChannels == 2)
    {
        return evaluateFilterStereo(dest, src, size);
    }
    else
#endif // USE_MULTICH_ALWAYS
    {
        assert(numChannels > 0);
        return evaluateFilterMulti(dest, src, size, numChannels);
    }
}

// Operator 'new' is overloaded so that it automatically creates a suitable instance 
// depending on if we've a MMX-capable CPU available or not.
void * FIRFilter::operator new(size_t s)
{
    // Notice! don't use "new FIRFilter" directly, use "newInstance" to create a new instance instead!
    ST_THROW_RT_ERROR("Error in FIRFilter::new: Don't use 'new FIRFilter', use 'newInstance' member instead!");
    return newInstance();
}
FIRFilter * FIRFilter::newInstance(){
    auto uExtensions = detectCPUextensions();
    // Check if MMX/SSE instruction set extensions supported by CPU
#ifdef SOUNDTOUCH_ALLOW_SSE
    if (uExtensions & SUPPORT_SSE)
        // SSE support
        return ::new FIRFilterSSE;
    else
#endif // SOUNDTOUCH_ALLOW_SSE
        // ISA optimizations not supported, use plain C version
        return ::new FIRFilter;
}
