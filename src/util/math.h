#ifndef MATH_H
#define MATH_H

// Causes MSVC to define M_PI and friends.
// http://msdn.microsoft.com/en-us/library/4hwaceh6.aspx
// Our SConscript defines this but check anyway.
#ifdef __WINDOWS__
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#endif

#include <initializer_list>
#include <limits>
#include <climits>
#include <algorithm>
#include <functional>
#include <utility>
#include <numeric>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <math.h>
// Note: Because of our fpclassify hack, we actualy need to inlude both,
// the c and the c++ version of the math header.
// From GCC 6.1.1 math.h depends on cmath, which failes to compile if included
// after our fpclassify hack


#include "util/assert.h"
#include "util/fpclassify.h"

// If we don't do this then we get the C90 fabs from the global namespace which
// is only defined for double.
using std::fabs;

#define math_max std::max
#define math_min std::min
#define math_maxn(...) std::max({ __VA_ARGS__})
#define math_minn(...) std::max({ __VA_ARGS__})
#define math_max3(a,b,c) math_maxn((a),(b),(c))

// Restrict value to the range [min, max]. Undefined behavior if min > max.
template <typename T>
constexpr T math_clamp(T val, T minval, T maxval) {
    return std::max(minval,std::min(maxval,val));
}

// NOTE(rryan): It is an error to call even() on a floating point number. Do not
// hack this to support floating point values! The programmer should be required
// to manually convert so they are aware of the conversion.
template <typename T>
constexpr std::enable_if_t<std::is_integral<T>::value,bool> even(T value) { return value % 2 == 0; }

template<class T>
constexpr std::enable_if_t<std::is_integral<T>::value,T>
roundUpToPowerOf2(T x)
{
    using U = std::make_unsigned_t<T>;
    auto u = U(x) - U(1);
    for(auto i = 1; i < (std::numeric_limits<U>::digits/2); i<<=1)
        u |= (u>>i);
    return T(u+U(1));
}
template <typename T>
constexpr T ratio2db(T a) { return std::log10(a) * 20; }

template <typename T>
constexpr  T db2ratio(T a) { return std::pow(10, a / 20); }
#endif /* MATH_H */
