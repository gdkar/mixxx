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
