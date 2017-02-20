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

#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>

namespace MathUtilities {

void getAlphaNorm(const double *data, unsigned int len, unsigned int alpha, double* ANorm)
{
    unsigned int i;
    double temp = 0.0;
    double a=0.0;

    for( i = 0; i < len; i++)
    {
	temp = data[ i ];

	a  += std::pow( std::abs(temp), double(alpha) );
    }
    a /= ( double )len;
    a = std::pow( a, ( 1.0 / (double) alpha ) );

    *ANorm = a;
}

double getAlphaNorm( const std::vector <double> &data, unsigned int alpha )
{
    unsigned int i;
    unsigned int len = data.size();
    double temp = 0.0;
    double a=0.0;

    for( i = 0; i < len; i++)
    {
	temp = data[ i ];

	a  += std::pow( std::abs(temp), double(alpha) );
    }
    a /= ( double )len;
    a = std::pow( a, ( 1.0 / (double) alpha ) );

    return a;
}
double median(const double *src, unsigned int len)
{
    if (len == 0) return 0;

    std::vector<double> scratch;
    for (int i = 0; i < len; ++i) scratch.push_back(src[i]);
    std::sort(scratch.begin(), scratch.end());

    int middle = len/2;
    if (len % 2 == 0) {
        return (scratch[middle] + scratch[middle - 1]) / 2;
    } else {
        return scratch[middle];
    }
}

double sum(const double *src, unsigned int len)
{
    unsigned int i ;
    double retVal =0.0;

    for(  i = 0; i < len; i++)
    {
	retVal += src[ i ];
    }

    return retVal;
}

double mean(const double *src, unsigned int len)
{
    double retVal =0.0;

    if (len == 0) return 0;

    double s = sum( src, len );

    retVal =  s  / (double)len;

    return retVal;
}

double mean(const std::vector<double> &src,
                           unsigned int start,
                           unsigned int count)
{
    double sum = 0.;

    if (count == 0) return 0;

    for (int i = 0; i < (int)count; ++i)
    {
        sum += src[start + i];
    }

    return sum / count;
}
int compareInt (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

void normalise(double *data, int length, NormaliseType type)
{
    switch (type) {

    case NormaliseNone: return;

    case NormaliseUnitSum:
    {
        double sum = 0.0;
        for (int i = 0; i < length; ++i) {
            sum += data[i];
        }
        if (sum != 0.0) {
            for (int i = 0; i < length; ++i) {
                data[i] /= sum;
            }
        }
    }
    break;

    case NormaliseUnitMax:
    {
        double max = 0.0;
        for (int i = 0; i < length; ++i) {
            if (fabs(data[i]) > max) {
                max = std::abs(data[i]);
            }
        }
        if (max != 0.0) {
            for (int i = 0; i < length; ++i) {
                data[i] /= max;
            }
        }
    }
    break;

    }
}

void normalise(std::vector<double> &data, NormaliseType type)
{
    switch (type) {

    case NormaliseNone: return;

    case NormaliseUnitSum:
    {
        double sum = 0.0;
        for (int i = 0; i < (int)data.size(); ++i)
            sum += data[i];
        if (sum != 0.0) {
            auto sum_inv = 1/sum;
            for (int i = 0; i < (int)data.size(); ++i)
                data[i] *= sum_inv;
        }
    }
    break;

    case NormaliseUnitMax:
    {
        auto _max = *std::max_element(data.cbegin(), data.cend());
        if (_max) {
            auto max_inv = 1/_max;
            for (int i = 0; i < (int)data.size(); ++i)
                data[i] *= max_inv;
        }
    }
    break;

    }
}

void adaptiveThreshold(std::vector<double> &data)
{
    int sz = int(data.size());
    if (sz == 0) return;

    std::vector<double> smoothed(sz);

    int p_pre = 8;
    int p_post = 7;

    for (int i = 0; i < sz; ++i) {

        int first = std::max(0,      i - p_pre);
        int last  = std::min(sz - 1, i + p_post);

        smoothed[i] = mean(data, first, last - first + 1);
    }

    for (int i = 0; i < sz; i++) {
        data[i] -= smoothed[i];
        if (data[i] < 0.0)
            data[i] = 0.0;
    }
}
double
factorial(int x)
{
    if (x < 0)
        return 0;
    auto f = 1.0;
    for (int i = 1; i <= x; ++i) {
	f *= double(i);
    }
    return f;
}
}
