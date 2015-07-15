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
#include <numeric>
#include <utility>
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
    float a=0.0;
    for(auto  i = 0; i < len; i++)
    {
	const auto temp = data[ i ];
	a  += ::powf( fabs(temp), float(alpha) );
    }
    a /= ( float )len;
    a = ::powf( a, ( 1.0f / (float) alpha ) );
    *ANorm = a;
}
float MathUtilities::getAlphaNorm( const std::vector <float> &data, unsigned int alpha )
{
    auto len = data.size();
    float a=0.0;
    for(auto  i = 0; i < len; i++)
    {
	auto temp = data[ i ];
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
    std::vector<float> scratch(len);
    for (auto i = 0; i < len; i++ ){scratch[i] = src[i];}
    std::sort(scratch.begin(),scratch.end());
    int middle;
    if ( len % 2 == 0 ){
	middle = len / 2;
	return ( scratch[middle] + scratch[middle - 1] ) *0.5f;
    }else{
	middle = len>>1;
	return  scratch[middle];
    }
}
float MathUtilities::sum(const float *src, unsigned int len){return std::accumulate(src,src+len,0.f);}
float MathUtilities::mean(const float *src, unsigned int len){return std::accumulate(src,src+len,0.f)*(1.f/len);}
float MathUtilities::mean(const std::vector<float> &src,
                           unsigned int start,
                           unsigned int count)
{
    return std::accumulate(src.begin()+start,src.begin()+start+count,0.f)*(1.f/count);
}
void MathUtilities::getFrameMinMax(const float *data, unsigned int len, float *min, float *max)
{
    auto pair = std::minmax_element(data,data+len);
    *min = *pair.first;
    *max = *pair.second;
}

int MathUtilities::getMax( float* pData, unsigned int Length, float* pMax )
{
        auto it = std::max_element(pData,pData+Length);
        if(pMax)*pMax=*it;
        return it-pData;
}
int MathUtilities::getMax( const std::vector<float> & data, float* pMax )
{
        auto it = std::max_element(data.begin(),data.end());
        if(pMax)*pMax=*it;
        return std::distance(data.begin(),it);
}

void MathUtilities::circShift( float* pData, int length, int shift)
{
        std::rotate(pData,pData+(shift%length),pData+length);
}
int MathUtilities::compareInt (const void * a, const void * b){return ( *(int*)a - *(int*)b );}
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

