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
#include <numeric>
#include <algorithm>
#include <utility>
#include <limits>
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
template<class... Args>
constexpr auto math_max3(Args&&... args)
{
    return std::max({args...});
}
#ifndef __cpp_lib_clamp
template<class T, class Compare>
constexpr const T& clamp( const T& v, const T& lo, const T& hi, Compare && comp)
{
    return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}
template<class T>
constexpr const T &clamp( const T& v, const T& lo, const T& hi)
{
    return clamp( v, lo, hi, std::less<>{});
}// Restrict value to the range [min, max]. Undefined behavior if min > max.
#else
using std::clamp;
#endif
template <typename T>
constexpr T math_clamp(T value, T _min, T _max) {
    // DEBUG_ASSERT compiles out in release builds so it does not affect
    // vectorization or pipelining of clamping in tight loops.
    return clamp(value, _min,_max);
}
#define math_maxn(...) std::max({ __VA_ARGS__})
#define math_minn(...) std::max({ __VA_ARGS__})
#define math_max3(a,b,c) math_maxn((a),(b),(c))


// NOTE(rryan): It is an error to call even() on a floating point number. Do not
// hack this to support floating point values! The programmer should be required
// to manually convert so they are aware of the conversion.
template <typename T>
constexpr std::enable_if_t<std::is_integral<T>::value,bool> even(T value) { return value % 2 == 0; }

namespace builtin {
    template<class T> struct _clz{};
    template<> struct _clz<uint32_t> {
        using restype = int;
        constexpr restype operator()(uint32_t x){ return __builtin_clz(x);}
    };
    template<> struct _clz<int32_t> {
        using restype = int;
        constexpr restype operator()(int32_t x){ return __builtin_clz(x);}
    };
    template<> struct _clz<uint64_t> {
        using restype = int;
        constexpr restype operator()(uint64_t x){ return __builtin_clzl(x);}
    };
    template<> struct _clz<int64_t> {
        using restype = int;
        constexpr restype operator()(int64_t x){ return __builtin_clzl(x);}
    };
};
template<class T>
constexpr typename builtin::_clz<T>::restype clz(T t)
{
    return builtin::_clz<T>{}(t);
}
template<class T>
constexpr int _digits() { return std::numeric_limits<T>::digits;}

template<class T>
constexpr std::enable_if_t<std::is_integral<T>::value,int> _bits() { return _digits<std::make_unsigned_t<T>>();}

template<class T>
constexpr typename builtin::_clz<T>::restype ilog2(T x)
{
    return _bits<T>() - clz(x-1);
}
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
