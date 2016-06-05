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

#ifndef CORRELATION_H
#define CORRELATION_H

#include <cmath>
#include <limits>
#include <climits>
#include <cfloat>
#include <numeric>
#include <algorithm>

namespace Correlation {
    template<class ForwardIt >
    void UnbiasedAutoCorrelate(ForwardIt sbeg, ForwardIt send, ForwardIt dst)
    {
        using restype = typename std::remove_reference<decltype(*sbeg)>::type;
        for(auto curr = sbeg; curr != send; curr++,dst++)
            *dst= std::max(std::numeric_limits<restype>::epsilon(),std::inner_product(curr, send, sbeg, restype{0}) / std::distance(curr,send));
    }
};

#endif // 
