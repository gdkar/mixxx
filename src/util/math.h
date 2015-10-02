_Pragma("once")
// Causes MSVC to define M_PI and friends.
// http://msdn.microsoft.com/en-us/library/4hwaceh6.aspx
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include "util/assert.h"
// If we don't do this then we get the C90 fabs from the global namespace which
// is only defined for double.
#define math_max std::max
#define math_min std::min
#define math_max3(a, b, c) math_max(math_max((a), (b)), (c))

// Restrict value to the range [min, max]. Undefined behavior if min > max.
template <typename T>
T math_clamp(T value, T min, T max) {
    // DEBUG_ASSERT compiles out in release builds so it does not affect
    // vectorization or pipelining of clamping in tight loops.
    return std::max<T>(min, std::min<T>(max, value));
}
// NOTE(rryan): It is an error to call even() on a floating point number. Do not
// hack this to support floating point values! The programmer should be required
// to manually convert so they are aware of the conversion.
template <typename T>
bool even(T value) {return value % 2 == 0; }
template<typename T>
constexpr T roundUpToPowerOf2(T v)
{
  DEBUG_ASSERT(v < std::numeric_limits<T>::max()/2);
  if(v<=0) return 1;
  v--;
  for(auto i = size_t{1}; i < 8 * sizeof(T);i<<=1) v|=(v>>i);
  return v+1;
}
// MSVS 2013 (_MSC_VER 1800) introduced C99 support.
template <typename T>
constexpr T ratio2db(T a) {return std::log10(a) * 20;}
template <typename T>
constexpr T db2ratio(T a) {return std::pow(10, a / 20);}
