////////////////////////////////////////////////////////////////////////////////
///
/// Peak detection routine. 
///
/// The routine detects highest value on an array of values and calculates the 
/// precise peak location as a mass-center of the 'hump' around the peak value.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2012-12-28 14:52:47 -0500 (Fri, 28 Dec 2012) $
// File revision : $Revision: 4 $
//
// $Id: PeakFinder.cpp 164 2012-12-28 19:52:47Z oparviai $
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

#include <math.h>
#include <assert.h>

#include "PeakFinder.h"

using namespace soundtouch;

#define max(x, y) (((x) > (y)) ? (x) : (y))


PeakFinder::PeakFinder()
  :minPos(0),
   maxPos(0)
{}


// Finds real 'top' of a peak hump from neighnourhood of the given 'peakpos'.
int PeakFinder::findTop(const float *data, int peakpos) const
{
    auto refvalue = data[peakpos];
    // seek within ±10 points
    auto start = peakpos - 10;
    if (start < minPos) start = minPos;
    auto end = peakpos + 10;
    if (end > maxPos) end = maxPos;
    for (auto i = start; i <= end; i ++){
        if (data[i] > refvalue){
            peakpos = i;
            refvalue = data[i];
        }
    }
    // failure if max value is at edges of seek range => it's not peak, it's at slope.
    if ((peakpos == start) || (peakpos == end)) return 0;
    return peakpos;
}
// Finds 'ground level' of a peak hump by starting from 'peakpos' and proceeding
// to direction defined by 'direction' until next 'hump' after minimum value will 
// begin
int PeakFinder::findGround(const float *data, int peakpos, int direction) const{

    auto climb_count = 0;
    auto refvalue = data[peakpos];
    auto lowpos = peakpos;
    auto pos = peakpos;
    while ((pos > minPos+1) && (pos < maxPos-1))
    {
        auto prevpos = pos;
        pos += direction;
        // calculate derivate
        auto delta = data[pos] - data[prevpos];
        if (delta <= 0){
            // going downhill, ok
            if (climb_count){
                climb_count --;  // decrease climb count
            }
            // check if new minimum found
            if (data[pos] < refvalue){
                // new minimum found
                lowpos = pos;
                refvalue = data[pos];
            }
        }else{
            // going uphill, increase climbing counter
            climb_count ++;
            if (climb_count > 5) break;    // we've been climbing too long => it's next uphill => quit
        }
    }
    return lowpos;
}
// Find offset where the value crosses the given level, when starting from 'peakpos' and
// proceeds to direction defined in 'direction'
int PeakFinder::findCrossingLevel(const float *data, float level, int peakpos, int direction) const
{
    auto peaklevel = data[peakpos];
    assert(peaklevel >= level);
    auto pos = peakpos;
    while ((pos >= minPos) && (pos < maxPos)){
        if (data[pos + direction] < level) return pos;   // crossing found
        pos += direction;
    }
    return -1;  // not found
}
// Calculates the center of mass location of 'data' array items between 'firstPos' and 'lastPos'
float PeakFinder::calcMassCenter(const float *data, int firstPos, int lastPos) const
{
    auto sum = (float)0;
    auto wsum = (float)0;
    for (auto i = firstPos; i <= lastPos; i ++)
    {
        sum += (float)i * data[i];
        wsum += data[i];
    }
    if (wsum < 1e-6f) return 0;
    return sum / wsum;
}



/// get exact center of peak near given position by calculating local mass of center
float PeakFinder::getPeakCenter(const float *data, int peakpos) const
{
    float peakLevel;            // peak level
    int crosspos1, crosspos2;   // position where the peak 'hump' crosses cutting level
    float cutLevel;             // cutting value
    float groundLevel;          // ground level of the peak
    int gp1, gp2;               // bottom positions of the peak 'hump'

    // find ground positions.
    gp1 = findGround(data, peakpos, -1);
    gp2 = findGround(data, peakpos, 1);

    groundLevel = 0.5f * (data[gp1] + data[gp2]);
    peakLevel = data[peakpos];

    // calculate 70%-level of the peak
    cutLevel = 0.70f * peakLevel + 0.30f * groundLevel;
    // find mid-level crossings
    crosspos1 = findCrossingLevel(data, cutLevel, peakpos, -1);
    crosspos2 = findCrossingLevel(data, cutLevel, peakpos, 1);

    if ((crosspos1 < 0) || (crosspos2 < 0)) return 0;   // no crossing, no peak..

    // calculate mass center of the peak surroundings
    return calcMassCenter(data, crosspos1, crosspos2);
}



float PeakFinder::detectPeak(const float *data, int aminPos, int amaxPos) 
{

    int i;
    int peakpos;                // position of peak level
    float highPeak, peak;

    this->minPos = aminPos;
    this->maxPos = amaxPos;

    // find absolute peak
    peakpos = minPos;
    peak = data[minPos];
    for (i = minPos + 1; i < maxPos; i ++)
    {
        if (data[i] > peak) 
        {
            peak = data[i];
            peakpos = i;
        }
    }
    
    // Calculate exact location of the highest peak mass center
    highPeak = getPeakCenter(data, peakpos);
    peak = highPeak;

    // Now check if the highest peak were in fact harmonic of the true base beat peak 
    // - sometimes the highest peak can be Nth harmonic of the true base peak yet 
    // just a slightly higher than the true base

    for (i = 3; i < 10; i ++)
    {
        float peaktmp, harmonic;
        int i1,i2;

        harmonic = (float )i * 0.5f;
        peakpos = (int)(highPeak / harmonic + 0.5f);
        if (peakpos < minPos) break;
        peakpos = findTop(data, peakpos);   // seek true local maximum index
        if (peakpos == 0) continue;         // no local max here

        // calculate mass-center of possible harmonic peak
        peaktmp = getPeakCenter(data, peakpos);

        // accept harmonic peak if 
        // (a) it is found
        // (b) is within ±4% of the expected harmonic interval
        // (c) has at least half x-corr value of the max. peak

        float diff = harmonic * peaktmp / highPeak;
        if ((diff < 0.96) || (diff > 1.04)) continue;   // peak too afar from expected

        // now compare to highest detected peak
        i1 = (int)(highPeak + 0.5);
        i2 = (int)(peaktmp + 0.5);
        if (data[i2] >= 0.4*data[i1])
        {
            // The harmonic is at least half as high primary peak,
            // thus use the harmonic peak instead
            peak = peaktmp;
        }
    }

    return peak;
}
