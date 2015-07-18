/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008 QMUL

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "KLDivergence.h"

#include <cmath>

float KLDivergence::distanceGaussian(const vector<float> &m1,
                                      const vector<float> &v1,
                                      const vector<float> &m2,
                                      const vector<float> &v2)
{
    int sz = m1.size();

    float d = -2.0 * sz;
    float small = 1e-20;
    for (int k = 0; k < sz; ++k) {
        float kv1 = v1[k] + small;
        float kv2 = v2[k] + small;
        float km = (m1[k] - m2[k]) + small;
        d += kv1 / kv2 + kv2 / kv1;
        d += km * (1.0 / kv1 + 1.0 / kv2) * km;
    }
    d *= 0.5;
    return d;
}
float KLDivergence::distanceDistribution(const vector<float> &d1,
                                          const vector<float> &d2,
                                          bool symmetrised)
{
    int sz = d1.size();
    float d = 0;
    float small = 1e-20f;
    for (int i = 0; i < sz; ++i) {d += d1[i] * log10((d1[i] + small) / (d2[i] + small));}
    if (symmetrised) {d += distanceDistribution(d2, d1, false);}

    return d;
}

