#ifndef MATH_H
#define MATH_H

// Causes MSVC to define M_PI and friends.
// http://msdn.microsoft.com/en-us/library/4hwaceh6.aspx
#define _USE_MATH_DEFINES
#include <ctgmath>
#include <limits>
#include <cfloat>
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

#ifdef _MSC_VER
// VC++ uses _isnan() instead of isnan() and !_finite instead of isinf.
#include <float.h>
#define isnan(x) _isnan(x)
#define isinf(x) (!_finite(x))
// Ask VC++ to emit an intrinsic for fabs instead of calling std::fabs.
#pragma intrinsic(fabs)
#else
#define isnan(x) std::isnan(x)
#define isinf(x) std::isinf(x)
#endif



template<typename T>
constexpr T roundUpToPowerOf2(T x)
{
  x--;
  x|=x>>1;
  x|=x>>2;
  x|=x>>4;
  x|=(sizeof(T)>1)?x>>8:0;
  x|=(sizeof(T)>2)?x>>16:0;
  x|=(sizeof(T)>4)?x>>32:0;
  return x+1;
}
/*inline int roundUpToPowerOf2(int v) {
    int power = 1;
    while (power < v && power > 0) {
        power *= 2;
    }
    // There is not a power of 2 higher than v representable by our
    // architecture's integer size.
    if (power < 0) {
        return -1;
    }
    return power;
}*/
template <typename T>
inline const T ratio2db(const T a) {
    return log10(a) * 20;
}

template <typename T>
inline const T db2ratio(const T a) {
    return pow(10, a / 20);
}

#endif /* MATH_H */
