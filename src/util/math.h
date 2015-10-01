_Pragma("once")
// Causes MSVC to define M_PI and friends.
// http://msdn.microsoft.com/en-us/library/4hwaceh6.aspx
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#include "util/assert.h"

template<typename T>
constexpr T max3(T a,T b,T c)
{
  return std::max<T>(std::max<T>(a,b),c);
}
template<typename T>
constexpr T max4(T a,T b,T c,T d)
{
  return std::max<T>(std::max<T>(a,b),std::max<T>(c,d));
}
template <typename S, typename T>
constexpr S clamp(S value, T min_val, T max_val) {
    return static_cast<S>(std::max<T>(min_val, std::min<T>(max_val, static_cast<T>(value))));
}
// NOTE(rryan): It is an error to call even() on a floating point number. Do not
// hack this to support floating point values! The programmer should be required
// to manually convert so they are aware of the conversion.
template <typename T>
constexpr bool even(T value) { return value % 2 == 0; }
template<typename T>
constexpr T roundUpToPowerOf2(T arg)
{
  arg--;
  for(auto shift = size_t{1}; shift < 8 * sizeof(T); shift <<=1) arg |= arg>>shift;
  return arg+1;
}
template <typename T>
constexpr T ratio2db(const T a) { return std::log10(a) * 20; }
template <typename T>
constexpr T db2ratio(const T a) { return std::pow(10, a / 20); }
