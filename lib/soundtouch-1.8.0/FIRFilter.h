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
// $Id: FIRFilter.h 201 2014-10-08 15:26:57Z oparviai $
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

#ifndef FIRFilter_H
#define FIRFilter_H

#include <stddef.h>
#include "STTypes.h"

namespace soundtouch
{

class FIRFilter 
{
protected:
    // Number of FIR filter taps
    uint length;    
    // Number of FIR filter taps divided by 8
    uint lengthDiv8;
    // Result divider factor in 2^k format
    uint resultDivFactor;
    // Result divider value.
    CSAMPLE resultDivider;
    // Memory for filter coefficients
    CSAMPLE *filterCoeffs;
    // Memory for keeping temporary sums in multichannel processing
    LONG_CSAMPLE *sum;
    uint sumsize;
    virtual uint evaluateFilterStereo(CSAMPLE *dest, const CSAMPLE *src, uint size) const;
    virtual uint evaluateFilterMono(CSAMPLE *dest, const CSAMPLE *src, uint size) const;
    virtual uint evaluateFilterMulti(CSAMPLE *dest, const CSAMPLE *src, uint size, uint numChannels);
public:
    FIRFilter();
    virtual ~FIRFilter();
    /// Operator 'new' is overloaded so that it automatically creates a suitable instance 
    /// depending on if we've a MMX-capable CPU available or not.
    static void * operator new(size_t s);
    static FIRFilter *newInstance();
    /// Applies the filter to the given sequence of samples. 
    /// Note : The amount of outputted samples is by value of 'filter_length' 
    /// smaller than the amount of input samples.
    ///
    /// \return Number of samples copied to 'dest'.
    uint evaluate(CSAMPLE *dest, const CSAMPLE *src, uint size, uint numChannels);
    uint getLength() const;
    virtual void setCoefficients(const CSAMPLE *coeffs, uint newLength, uint uResultDivFactor);
};
    /// Class that implements SSE optimized functions exclusive for floating point samples type.
    class FIRFilterSSE : public FIRFilter
    {
    protected:
        float *filterCoeffsUnalign;
        float *filterCoeffsAlign;

        virtual uint evaluateFilterStereo(float *dest, const float *src, uint size) const;
    public:
        FIRFilterSSE();
        ~FIRFilterSSE();

        virtual void setCoefficients(const float *coeffs, uint newLength, uint uResultDivFactor);
    };
}

#endif  // FIRFilter_H
