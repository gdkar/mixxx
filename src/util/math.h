_Pragma("once")
// Causes MSVC to define M_PI and friends.
// http://msdn.microsoft.com/en-us/library/4hwaceh6.aspx
// Our SConscript defines this but check anyway.
#ifdef __WINDOWS__
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#endif
#include <cmath>
#include <algorithm>

#include "util/assert.h"

// If we don't do this then we get the C90 fabs from the global namespace which
// is only defined for double.
using std::fabs;

#define math_max std::max
#define math_min std::min
#define math_max3(a, b, c) math_max(math_max((a), (b)), (c))

// Restrict value to the range [min, max]. Undefined behavior if min > max.
template <typename T>
inline T math_clamp(T value, T min, T max) {
    // DEBUG_ASSERT compiles out in release builds so it does not affect
    // vectorization or pipelining of clamping in tight loops.
    DEBUG_ASSERT(min <= max);
    return math_max(min, math_min(max, value));
}

// NOTE(rryan): It is an error to call even() on a floating point number. Do not
// hack this to support floating point values! The programmer should be required
// to manually convert so they are aware of the conversion.
template <typename T>
inline bool even(T value) {
    return value % 2 == 0;
}
namespace {
    template<typename T>
    inline constexpr T roundup_(T x, size_t shift)
    {
        return (shift < sizeof(T) * 8) ? roundup_(x|(x>>shift),shift<<1) : x;
    }
};
template<typename T>
constexpr T roundup(T x)
{
    return roundup_(x - T{1}, size_t{1}) + T{1};
}

template <typename T>
inline const T ratio2db(const T a) {
    return std::log10(a) * 20;
}

template <typename T>
inline const T db2ratio(const T a) {
    return std::pow(10, a / 20);
}
