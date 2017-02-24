/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2015 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#ifndef _RUBBERBAND_SYSUTILS_H_
#define _RUBBERBAND_SYSUTILS_H_

#include <cstdint>
#include <cstdarg>
#include <cinttypes>
#include <cstring>
#include <csignal>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <climits>
#include <cfloat>

#include <exception>
#include <stdexcept>
#include <chrono>
#include <numeric>
#include <memory>
#include <algorithm>
#include <iterator>
#include <functional>
#include <initializer_list>
#include <ratio>
#include <thread>
#include <atomic>
#include <mutex>
#include <utility>
#include <limits>
#include <type_traits>
#include <tuple>
#include <cmath>
#include <vector>
#include <complex>
#include <array>
#include <string>
#include <iostream>
#include <deque>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

#include <alloca.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <dlfcn.h>

#include "Simd.hpp"
#include "Math.hpp"
#include "VectorOps.hpp"
#include "VectorOpsComplex.hpp"

namespace RBMixxxVamp {
template<class T>
constexpr std::enable_if_t<bs::is_scalar<T>{},T> princarg(T a)
{
    return std::remainder(a, bs::Twopi<T>());
}

template<class T>
constexpr std::enable_if_t<(bs::cardinal_of<T>{}>1ul),T> princarg(T a)
{
    using R = bs::real_of_t<T>;
    constexpr auto w = int(bs::cardinal_of_t<T>{});
    auto ret = T{0};
    for(auto i = 0; i < w; ++i)
        ret[i] = std::remainder(a[i],bs::Twopi<R>());
    return ret;
}

} // end namespace

#endif