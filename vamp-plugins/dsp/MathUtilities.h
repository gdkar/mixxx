/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file 2005-2006 Christian Landone.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef MATHUTILITIES_H
#define MATHUTILITIES_H

// M_PI needs to be difined for Windows builds
#ifndef M_PI
#define M_PI    3.14159265358979323846f
#endif

#include <vector>

#include "nan-inf.h"

class MathUtilities  
{
public:	
    static float round( float x );

    static void	  getFrameMinMax( const float* data, unsigned int len,  float* min, float* max );

    static float mean( const float* src, unsigned int len );
    static float mean( const std::vector<float> &data,
                        unsigned int start, unsigned int count );
    static float sum( const float* src, unsigned int len );
    static float median( const float* src, unsigned int len );

    static float princarg( float ang );
    static float mod( float x, float y);

    static void	  getAlphaNorm(const float *data, unsigned int len, unsigned int alpha, float* ANorm);
    static float getAlphaNorm(const std::vector <float> &data, unsigned int alpha );

    static void   circShift( float* data, int length, int shift);

    static int	  getMax( float* data, unsigned int length, float* max = 0 );
    static int	  getMax( const std::vector<float> &data, float* max = 0 );
    static int    compareInt(const void * a, const void * b);

    enum NormaliseType {
        NormaliseNone,
        NormaliseUnitSum,
        NormaliseUnitMax
    };

    static void   normalise(float *data, int length,
                            NormaliseType n = NormaliseUnitMax);

    static void   normalise(std::vector<float> &data,
                            NormaliseType n = NormaliseUnitMax);

    // moving mean threshholding:
    static void adaptiveThreshold(std::vector<float> &data);

    static bool isPowerOfTwo(int x);
    static int nextPowerOfTwo(int x); // e.g. 1300 -> 2048, 2048 -> 2048
    static int previousPowerOfTwo(int x); // e.g. 1300 -> 1024, 2048 -> 2048
    static int nearestPowerOfTwo(int x); // e.g. 1300 -> 1024, 1700 -> 2048
};

#endif
