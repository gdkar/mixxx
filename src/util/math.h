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

#include <cmath> 
#include <math.h>

// Note: Because of our fpclassify hack, we actualy need to inlude both, 
// the c and the c++ version of the math header.  
// From GCC 6.1.1 math.h depends on cmath, which failes to compile if included 
// after our fpclassify hack 

#include <algorithm>
#include <type_traits>
#include <utility>

#include "util/assert.h"
#include "util/fpclassify.h"

// If we don't do this then we get the C90 fabs from the global namespace which
// is only defined for double.
using std::fabs;

#define math_max std::max
#define math_min std::min
#define math_max3(a, b, c) math_max(math_max((a), (b)), (c))

// Restrict value to the range [min, max]. Undefined behavior if min > max.
template <typename T>
constexpr T math_clamp(T midv, T minv, T maxv) { return std::max(std::min(midv,maxv),minv); }

// NOTE(rryan): It is an error to call even() on a floating point number. Do not
// hack this to support floating point values! The programmer should be required
// to manually convert so they are aware of the conversion.
template <typename T>
constexpr std::enable_if_t<std::is_integral<T>::value,bool>
even(T value) { return !(value & T{1}); }

template<class T>
constexpr std::enable_if_t<std::is_integral<T>::value,T>
roundUpToPowerOf2( T v)
{
    using U = std::make_unsigned_t<T>;
    auto u = static_cast<U>(v) - U{1};
    for(auto i = 1u; i < CHAR_BIT * sizeof(u); i <<= 1)
        u |= (u>>i);
    return static_cast<T>(u + U{1});

}


template <typename T>
constexpr const T ratio2db(const T a) { return std::log10(a) * T{20}; }

template <typename T>
constexpr const T db2ratio(const T a) { return std::pow(T{10}, a * T(1./ 20)); }

#endif /* MATH_H */
