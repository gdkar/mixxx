#ifndef TYPES_H
#define TYPES_H

#include <cstddef>
#include <climits>

#include "util/math.h"

// TODO(uklotzde): Replace 'const' with 'constexpr' after
// switching to Visual Studio 2015 on Windows.

// Signed integer type for POT array indices, sizes and pointer
// arithmetic. Its size (32-/64-bit) depends on the CPU architecture.
// This should be used for all CSAMLE operations since it is fast and
// allows compiler auto vectorizing. For Qt container operations use
// just int as before.
using SINT = std::ptrdiff_t;

// 16-bit integer sample data within the asymmetric
// range [SHRT_MIN, SHRT_MAX].
using SAMPLE = int16_t;
constexpr SAMPLE SAMPLE_ZERO = 0;
constexpr SAMPLE SAMPLE_MIN = SHRT_MIN;
constexpr SAMPLE SAMPLE_MAX = SHRT_MAX;

// Limits the range of a SAMPLE value to [SAMPLE_MIN, SAMPLE_MAX].

constexpr SAMPLE SAMPLE_clamp(SAMPLE in) {
    return math_clamp(in, SAMPLE_MIN, SAMPLE_MAX);
}

// Limits the range of a SAMPLE value to [-SAMPLE_MAX, SAMPLE_MAX].
constexpr SAMPLE SAMPLE_clampSymmetric(SAMPLE in) {
    return math_clamp(in, static_cast<SAMPLE>(-SAMPLE_MAX), SAMPLE_MAX);
}

// 32-bit single precision floating-point sample data
// normalized within the range [-1.0, 1.0] with a peak
// amplitude of 1.0. No min/max constants here to
// emphasize the symmetric value range of CSAMPLE
// data!
using CSAMPLE = float;
constexpr CSAMPLE CSAMPLE_ZERO = 0.0f;
constexpr CSAMPLE CSAMPLE_ONE = 1.0f;
constexpr CSAMPLE CSAMPLE_PEAK = CSAMPLE_ONE;

// Limits the range of a CSAMPLE value to [-CSAMPLE_PEAK, CSAMPLE_PEAK].

constexpr CSAMPLE CSAMPLE_clamp(CSAMPLE in) {
    return math_clamp(in, -CSAMPLE_PEAK, CSAMPLE_PEAK);
}

// Gain values for weighted calculations of CSAMPLE
// data in the range [0.0, 1.0]. Same data type as
// CSAMPLE to avoid type conversions in calculations.
using CSAMPLE_GAIN = CSAMPLE;
constexpr auto CSAMPLE_GAIN_ZERO = CSAMPLE_ZERO;
constexpr auto CSAMPLE_GAIN_ONE = CSAMPLE_ONE;
constexpr auto CSAMPLE_GAIN_MIN = CSAMPLE_GAIN_ZERO;
constexpr auto CSAMPLE_GAIN_MAX = CSAMPLE_GAIN_ONE;

// Limits the range of a CSAMPLE_GAIN value to [CSAMPLE_GAIN_MIN, CSAMPLE_GAIN_MAX].
constexpr CSAMPLE_GAIN CSAMPLE_GAIN_clamp(CSAMPLE_GAIN in) {
    return math_clamp(in, CSAMPLE_GAIN_MIN, CSAMPLE_GAIN_MAX);
}
template<class T>
constexpr std::add_const_t<T>& as_const(T & t) noexcept
{
    return t;
}


#endif /* TYPES_H */
