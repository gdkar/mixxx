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
#include <numeric>
#include <algorithm>
#include <utility>
#include <limits>
#include <math.h>
// Note: Because of our fpclassify hack, we actualy need to inlude both,
// the c and the c++ version of the math header.
// From GCC 6.1.1 math.h depends on cmath, which failes to compile if included
// after our fpclassify hack

#include <functional>
#include <type_traits>

#include "util/assert.h"
#include "util/fpclassify.h"

// If we don't do this then we get the C90 fabs from the global namespace which
// is only defined for double.
using std::fabs;

#define math_max std::max
#define math_min std::min
constexpr decltype(auto) math_max3(auto && a, auto && b, auto && c)
{
    return std::max({a,b,c});
}
template<class T,class Compare>
constexpr const T& clamp(const T& v, const T& lo, const T& hi, Compare comp)
{
    return comp(v,lo) ? lo : comp(hi,v) ? hi : v;
}
template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
    return clamp(v, lo, hi, std::less<>());
}

// Restrict value to the range [min, max]. Undefined behavior if min > max.
template <typename T>
constexpr T math_clamp(T value, T _min, T _max) {
    // DEBUG_ASSERT compiles out in release builds so it does not affect
    // vectorization or pipelining of clamping in tight loops.
    return clamp(value, _min,_max);
}

// NOTE(rryan): It is an error to call even() on a floating point number. Do not
// hack this to support floating point values! The programmer should be required
// to manually convert so they are aware of the conversion.
template <typename T>
constexpr typename std::enable_if<std::is_integral<T>::value,bool>::type even(T value)
{
    return !(value & T{1});
}

#ifdef _MSC_VER
// Ask VC++ to emit an intrinsic for fabs instead of calling std::fabs.
#pragma intrinsic(fabs)
#endif
namespace detail {
template<class T>
constexpr typename std::enable_if<std::is_integral<T>::value,T>::type roundUpToPowerOf2(T v, int shift)
{
    using U = typename std::make_unsigned<T>::type;
    return (shift < (std::numeric_limits<U>::digits>>1)) ? roundUpToPowerOf2(U(v)|(U(v)>>shift),shift<<1) : (U(v)|(U(v)>>shift));
}
}
template<class T>
constexpr typename std::enable_if<std::is_integral<T>::value,T>::type roundUpToPowerOf2(T v)
{
    using U = typename std::make_unsigned<T>::type;
    return T(U(1) + detail::roundUpToPowerOf2(U(v) - U(1),1));
}

template <typename T>
constexpr const T ratio2db(const T a) {
    return std::log10(a) * 20;
}
template <typename T>
constexpr const T db2ratio(const T a) {
    return std::pow(10, a / 20);
}
#endif /* MATH_H */
