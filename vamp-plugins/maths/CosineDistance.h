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
    auto acc0 = std::inner_product(beg0, end0, beg0 restype{0});
    auto acc1 = std::inner_product(beg1, beg1 + std::distance(beg0,end0), beg1, restype{0});
    auto accp = std::inner_product(beg0, end0, beg1, restype{0});
    return restype(1) - accp
        / std::sqrt(std::abs(acc0 * acc1) + std::numeric_limits<restype>::epsilon());
}

#endif

