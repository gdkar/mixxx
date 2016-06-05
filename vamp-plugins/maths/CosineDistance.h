/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2008 Kurt Jacobson.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef COSINEDISTANCE_H
#define COSINEDISTANCE_H

#include <cmath>
#include <limits>
#include <utility>
#include <numeric>
#include <iterator>
#include <algorithm>

template<class ForwardIt>
std::iterator_traits<ForwardIt>::value_type CosineDistance(ForwardIt beg0,ForwardIt end0, ForwardIt beg1)
{
    using restype = std::iterator_traits<ForwardIt>::value_type;
    auto acc0 = restype(0), acc1 = restype(0), accp = restype(0);
    for(auto cur0 = beg0, cur1 = beg1; cur0 != end0; cur0++,cur1++)
        accp += cur0 * cur1; acc0 += cur0 * cur0; acc1 += cur1 * cur1;

    return restype(1) - accp
        / std::sqrt(std::abs(acc0 * acc1) + std::numeric_limits<restype>::epsilon());
}

#endif

