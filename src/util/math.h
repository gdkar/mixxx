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
#include <limits>
#include <type_traits>
#include <algorithm>

#include <math.h>
// Note: Because of our fpclassify hack, we actually need to include both,
// the c and the c++ version of the math header.
// From GCC 6.1.1 math.h depends on cmath, which fails to compile if included
// after our fpclassify hack


#include "util/assert.h"

// If we don't do this then we get the C90 fabs from the global namespace which
// is only defined for double.
using std::fabs;

//#define math_max std::max
//#define math_min std::min
//#define math_max(math_max((a), (b)), (c))
template<typename... Args>
constexpr std::common_type_t<Args...> math_min(Args ...args)
{
    using U = std::common_type_t<Args...>;
    return std::min<U>({U(args)...});
}

template<typename... Args>
constexpr std::common_type_t<Args...> math_max(Args ...args)
{
    using U = std::common_type_t<Args...>;
    return std::max<U>({U(args)...});
}
// Restrict value to the range [min, max]. Undefined behavior if min > max.
template <typename T>
constexpr T math_clamp(T value, T min, T max) {
    // DEBUG_ASSERT compiles out in release builds so it does not affect
    // vectorization or pipelining of clamping in tight loops.
    DEBUG_ASSERT(min <= max);
    return math_max(min, math_min(max, value));
}

// NOTE(rryan): It is an error to call even() on a floating point number. Do not
// hack this to support floating point values! The programmer should be required
// to manually convert so they are aware of the conversion.
template <typename T>
constexpr bool even(T value) {
    return value % 2 == 0;
}

#ifdef _MSC_VER
// Ask VC++ to emit an intrinsic for fabs instead of calling std::fabs.
#pragma intrinsic(fabs)
#endif

constexpr int roundUpToPowerOf2(int v) {
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
}

constexpr double roundToFraction(double value, int denominator) {
    int wholePart = value;
    double fractionPart = value - wholePart;
    double numerator = std::round(fractionPart * denominator);
    return wholePart + numerator / denominator;
}

template <typename T>
constexpr T ratio2db(const T a) {
    static_assert(std::is_same<float, T>::value ||
                  std::is_same<double, T>::value ||
                  std::is_same<long double, T>::value,
                  "ratio2db works only for floating point types");
    return std::log10(a) * 20;
}

template <typename T>
constexpr T db2ratio(const T a) {
    static_assert(std::is_same<float, T>::value ||
                  std::is_same<double, T>::value ||
                  std::is_same<long double, T>::value,
                  "db2ratio works only for floating point types");
    return std::pow(10, a / 20);
}

#endif /* MATH_H */
