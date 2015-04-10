////////////////////////////////////////////////////////////////////////////////
/// 
/// Linear interpolation algorithm.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// $Id: InterpolateLinear.cpp 180 2014-01-06 19:16:02Z oparviai $
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

#include <assert.h>
#include <stdlib.h>
#include "InterpolateLinear.h"

using namespace soundtouch;



//////////////////////////////////////////////////////////////////////////////
//
// InterpolateLinear - floating point arithmetic implementation
// 
//////////////////////////////////////////////////////////////////////////////


// Constructor
InterpolateLinear::InterpolateLinear() : TransposerBase()
{
    // Notice: use local function calling syntax for sake of clarity, 
    // to indicate the fact that C++ constructor can't call virtual functions.
    resetRegisters();
    setRate(1.0f);
}


void InterpolateLinear::resetRegisters()
{
    fract = 0;
}


// Transposes the sample rate of the given samples using linear interpolation. 
// 'Mono' version of the routine. Returns the number of samples returned in 
// the "dest" buffer
int InterpolateLinear::transposeMono(CSAMPLE *dest, const CSAMPLE *src, int &srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        float out;
        assert(fract < 1.0);

        out = (1.0 - fract) * src[0] + fract * src[1];
        dest[i] = (CSAMPLE)out;
        i ++;

        // update position fraction
        fract += rate;
        // update whole positions
        int whole = (int)fract;
        fract -= whole;
        src += whole;
        srcCount += whole;
    }
    srcSamples = srcCount;
    return i;
}


// Transposes the sample rate of the given samples using linear interpolation. 
// 'Mono' version of the routine. Returns the number of samples returned in 
// the "dest" buffer
int InterpolateLinear::transposeStereo(CSAMPLE *dest, const CSAMPLE *src, int &srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd)
    {
        float out0, out1;
        assert(fract < 1.0);

        out0 = (1.0 - fract) * src[0] + fract * src[2];
        out1 = (1.0 - fract) * src[1] + fract * src[3];
        dest[2*i]   = (CSAMPLE)out0;
        dest[2*i+1] = (CSAMPLE)out1;
        i ++;

        // update position fraction
        fract += rate;
        // update whole positions
        int whole = (int)fract;
        fract -= whole;
        src += 2*whole;
        srcCount += whole;
    }
    srcSamples = srcCount;
    return i;
}


int InterpolateLinear::transposeMulti(CSAMPLE *dest, const CSAMPLE *src, int &srcSamples)
{
    int i;
    int srcSampleEnd = srcSamples - 1;
    int srcCount = 0;

    i = 0;
    while (srcCount < srcSampleEnd){
    
        const float vol1 = (1.0f- fract);
        for (int c = 0; c < numChannels; c ++){
            float temp = vol1 * src[c] + fract * src[c + numChannels];
            *dest = (CSAMPLE)temp;
            dest ++;
        }
        i++;
        fract += rate;
        int iWhole = (int)fract;
        fract -= iWhole;
        srcCount += iWhole;
        src += iWhole * numChannels;
    }
    srcSamples = srcCount;

    return i;
}
