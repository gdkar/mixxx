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

#include "MathUtilities.h"

#include <algorithm>
#include <iostream>
#include <cmath>
#include <math.h>

float MathUtilities::mod(float x, float y)
{
    return x - ( y * floorf(x/y) );
}

float MathUtilities::princarg(float ang)
{
    float ValOut;

    ValOut = fmodf( ang + (float)M_PI, -(float)(2 * M_PI) ) + (float)M_PI;

    return ValOut;
}

void MathUtilities::getAlphaNorm(const float *data, unsigned int len, unsigned int alpha, float* ANorm)
{
    unsigned int i;
    float temp = 0.0;
    float a=0.0;
	
    for( i = 0; i < len; i++)
    {
	temp = data[ i ];
		
	a  += ::powf( fabs(temp), float(alpha) );
    }
    a /= ( float )len;
    a = ::powf( a, ( 1.0f / (float) alpha ) );

    *ANorm = a;
}

float MathUtilities::getAlphaNorm( const std::vector <float> &data, unsigned int alpha )
{
    unsigned int i;
    unsigned int len = data.size();
    float temp = 0.0;
    float a=0.0;
    for( i = 0; i < len; i++)
    {
	temp = data[ i ];
	a  += ::powf( fabs(temp), float(alpha) );
    }
    a /= ( float )len;
    a = ::powf( a, ( 1.0f / (float) alpha ) );
    return a;
}

float MathUtilities::round(float x)
{
    float val = (float)floorf(x + 0.5);
  
    return val;
}

float MathUtilities::median(const float *src, unsigned int len)
{
    unsigned int i, j;
    float tmp = 0.0;
    float tempMedian;
    float medianVal;
 
    float* scratch = new float[ len ];//Vector < float > sortedX = Vector < float > ( size );

    for ( i = 0; i < len; i++ )
    {
	scratch[i] = src[i];
    }

    for ( i = 0; i < len - 1; i++ )
    {
	for ( j = 0; j < len - 1 - i; j++ )
	{
	    if ( scratch[j + 1] < scratch[j] )
	    {
		// compare the two neighbors
		tmp = scratch[j]; // swap a[j] and a[j+1]
		scratch[j] = scratch[j + 1];
		scratch[j + 1] = tmp;
	    }
	}
    }
    int middle;
    if ( len % 2 == 0 )
    {
	middle = len / 2;
	tempMedian = ( scratch[middle] + scratch[middle - 1] ) / 2;
    }
    else
    {
	middle = ( int )floorf( len / 2.0 );
	tempMedian = scratch[middle];
    }
    medianVal = tempMedian;
    delete [] scratch;
    return medianVal;
}

float MathUtilities::sum(const float *src, unsigned int len)
{
    unsigned int i ;
    float retVal =0.0;

    for(  i = 0; i < len; i++) retVal += src[ i ];

    return retVal;
}

float MathUtilities::mean(const float *src, unsigned int len)
{
    float retVal =0.0;
    float s = sum( src, len );
    retVal =  s  / (float)len;
    return retVal;
}

float MathUtilities::mean(const std::vector<float> &src,
                           unsigned int start,
                           unsigned int count)
{
    float sum = 0.;
    for (int i = 0; i < count; ++i)
        sum += src[start + i];
    return sum / count;
}

void MathUtilities::getFrameMinMax(const float *data, unsigned int len, float *min, float *max)
{
    unsigned int i;
    if (len == 0) {
        *min = *max = 0;
        return;
    }
    float _min = data[0];
    float _max = data[0];
    for( i = 0; i < len; i++)
    {
        const float temp = data[i];
        _min = std::min(temp,_min);
        _max = std::max(temp,_max);
    }
    *min = _min;
    *max = _max;
}

int MathUtilities::getMax( float* pData, unsigned int Length, float* pMax )
{
	unsigned int index = 0;
	unsigned int i;
	
	auto max = pData[0];
	for( i = 0; i < Length; i++){
		const auto temp = pData[ i ];
                index = (temp>max)?i:index;
                max   = (temp>max)?temp:max;
   	}
	if (pMax) *pMax = max;
	return index;
}

int MathUtilities::getMax( const std::vector<float> & data, float* pMax )
{
	unsigned int index = 0;
	unsigned int i;
	float max = data[0];
	for( i = 0; i < data.size(); i++){
		const auto temp = data[ i ];
                index = (temp>max)?i:index;
                max   = (temp>max)?temp:max;
		
   	}
	if (pMax) *pMax = max;
	return index;
}

void MathUtilities::circShift( float* pData, int length, int shift)
{
	shift = shift % length;
	int i,n;
	for( i = 0; i < shift; i++){
		auto temp=*(pData + length - 1);
		for( n = length-2; n >= 0; n--){
			*(pData+n+1)=*(pData+n);
		}
                *pData = temp;
        }
}

int MathUtilities::compareInt (const void * a, const void * b){
  return ( *(int*)a - *(int*)b );
}

void MathUtilities::normalise(float *data, int length, NormaliseType type)
{
    switch (type) {
    case NormaliseNone: return;
    case NormaliseUnitSum:{
        auto sum = 0.0f;
        for (int i = 0; i < length; ++i) {
            sum += data[i];
        }
        if (sum != 0.0f) {
            sum = 1.f/sum;
            for (int i = 0; i < length; ++i) {
                data[i] *= sum;
            }
        }
    }
    break;
    case NormaliseUnitMax:{
        auto max = 0.f;
        for (int i = 0; i < length; ++i) {
            if (fabsf(data[i]) > max) {
                max = fabs(data[i]);
            }
        }
        if (max != 0.f) {
            max=1.f/max;
            for (int i = 0; i < length; ++i) {
                data[i] *= max;
            }
        }
    }
    break;
    }
}

void MathUtilities::normalise(std::vector<float> &data, NormaliseType type)
{
    switch (type) {

    case NormaliseNone: return;

    case NormaliseUnitSum:
    {
        float sum = 0.0;
        for (int i = 0; i < data.size(); ++i) sum += data[i];
        if (sum != 0.0) {
            for (int i = 0; i < data.size(); ++i) data[i] /= sum;
        }
    }
    break;

    case NormaliseUnitMax:
    {
        float max = 0.0;
        for (int i = 0; i < data.size(); ++i) {
            if (fabs(data[i]) > max) max = fabs(data[i]);
        }
        if (max != 0.0) {
            for (int i = 0; i < data.size(); ++i) data[i] /= max;
        }
    }
    break;

    }
}

void MathUtilities::adaptiveThreshold(std::vector<float> &data)
{
    int sz = int(data.size());
    if (sz == 0) return;
    std::vector<float> smoothed(sz);
    int p_pre = 8;
    int p_post = 7;
    for (int i = 0; i < sz; ++i) {

        int first = std::max(0,      i - p_pre);
        int last  = std::min(sz - 1, i + p_post);
        smoothed[i] = mean(data, first, last - first + 1);
    }

    for (int i = 0; i < sz; i++) {
        data[i] -= smoothed[i];
        if (data[i] < 0.0) data[i] = 0.0;
    }
}
bool
MathUtilities::isPowerOfTwo(int x){return ((x)&&!(x&(x-1)));}
int
MathUtilities::nextPowerOfTwo(int x)
{
    x--;x|=x>>1;x|=x>>2;x|=x>>4;x|=x>>8;x|=x>>16;return x+1;
}
int
MathUtilities::previousPowerOfTwo(int x){return isPowerOfTwo(x)?x:(nextPowerOfTwo(x)>>1);}
int
MathUtilities::nearestPowerOfTwo(int x){
    if (isPowerOfTwo(x)) return x;
    int n0 = previousPowerOfTwo(x), n1 = nextPowerOfTwo(x);
    if (x - n0 < n1 - x) return n0;
    else return n1;
}

