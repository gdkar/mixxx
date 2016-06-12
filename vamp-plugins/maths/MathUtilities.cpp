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

double MathUtilities::round(double x)
{
    if (x < 0) {
        return -floor(-x + 0.5);
    } else {
        return floor(x + 0.5);
    }
}

double MathUtilities::median(const double *src, unsigned int len)
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
void MathUtilities::getFrameMinMax(const double *data, unsigned int len, double *_min, double *_max)
{
    auto it = std::minmax_element(&data[0],&data[len]);
    if(_min) *_min = *it.first;
    if(_max) *_max = *it.second;
}
int MathUtilities::getMax( double* pData, unsigned int Length, double* pMax )
{
    auto it = std::max_element(pData,std::next(pData,Length));
    if(pMax)
        *pMax = *it;
    return std::distance(pData,it);
}
int MathUtilities::getMax( const std::vector<double> & data, double* pMax )
{
    auto it = std::max_element(data.cbegin(),data.cend());
    if(pMax)
        *pMax = *it;
    return std::distance(data.cbegin(),it);
}

void MathUtilities::circShift( double* pData, int length, int shift)
{
    std::rotate(&pData[0],&pData[shift],&pData[length]);
}

int MathUtilities::compareInt (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

void MathUtilities::normalise(double *data, int length, NormaliseType type)
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
                max = fabs(data[i]);
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

void MathUtilities::normalise(std::vector<double> &data, NormaliseType type)
{
    switch (type) {

    case NormaliseNone: return;

    case NormaliseUnitSum:
    {
        double sum = 0.0;
        for (int i = 0; i < (int)data.size(); ++i) sum += data[i];
        if (sum != 0.0) {
            for (int i = 0; i < (int)data.size(); ++i) data[i] /= sum;
        }
    }
    break;

    case NormaliseUnitMax:
    {
        double max = 0.0;
        for (int i = 0; i < (int)data.size(); ++i) {
            if (fabs(data[i]) > max) max = fabs(data[i]);
        }
        if (max != 0.0) {
            for (int i = 0; i < (int)data.size(); ++i)
                data[i] /= max;
        }
    }
    break;

    }
}

void MathUtilities::adaptiveThreshold(std::vector<double> &data)
{
    int sz = int(data.size());
    if (sz == 0) return;

    std::vector<double> smoothed(sz);
	
    int p_pre = 8;
    int p_post = 7;

    for (int i = 0; i < sz; ++i) {

        int first = std::max(0,      i - p_pre);
        int last  = std::min(sz - 1, i + p_post);

        smoothed[i] = mean(std::next(data.cbegin(),first), std::next(data.cbegin(),last));;
    }
    for (int i = 0; i < sz; i++) {
        data[i] -= smoothed[i];
        if (data[i] < 0.0)
            data[i] = 0.0;
    }
}
