/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
    This file 2005-2006 Christian Landone.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef MATHUTILITIES_H
#define MATHUTILITIES_H

#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <type_traits>
#include <initializer_list>
#include <memory>
#include <iterator>
#include <limits>
#include <complex>
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <limits>

#include "nan-inf.h"

/**
 * Static helper functions for simple mathematical calculations.
 */
namespace MathUtilities
{
    /**
     * Round x to the nearest integer.
     */
    using std::round;
//    double round( double x );

    /**
     * Return through min and max pointers the highest and lowest
     * values in the given array of the given length.
     */
    void          getFrameMinMax( const double* data, unsigned int len,  double* min, double* max );

    /**
     * Return the mean of the given array of the given length.
     */
    template<class I>
    typename std::iterator_traits<I>::value_type mean(I src, size_t len)
    {
        using T = typename std::iterator_traits<I>::value_type;
        return len ? std::accumulate(src,src + len,T{})/len : T{};
    }

    /**
     * Return the mean of the subset of the given vector identified by
     * start and count.
     */
    template<class C>
    typename C::value_type mean(const C &data, size_t start, size_t count)
    {
        return mean(std::begin(data) + start, count);
    }

    /**
     * Return the sum of the values in the given array of the given
     * length.
     */
    template<class I>
     typename std::iterator_traits<I>::value_type sum( I src, size_t len )
     {
        using T = typename std::iterator_traits<I>::value_type;
        return std::accumulate(src,src+len,T{});
     }

    /**
     * Return the median of the values in the given array of the given
     * length. If the array is even in length, the returned value will
     * be half-way between the two values adjacent to median.
     */
    template<class I>
    typename std::iterator_traits<I>::value_type median( I src, size_t len)
    {
        using T = typename std::iterator_traits<I>::value_type;
        if (len == 0)
            return 0;
        auto scratch = std::vector<T>(src,src + len);
        auto s_beg = std::begin(scratch),s_mid = s_beg + (len/2),s_end=std::end(scratch);
        std::nth_element(s_beg,s_mid,s_end);
        if(len % 2) {
            auto s_mid2 = std::min_element(s_mid + 1,s_end);
            return (*s_mid + *s_mid2) * T(0.5);
        }else{
            return *s_mid;
        }
    }
    /**
     * The principle argument function. Map the phase angle ang into
     * the range [-pi,pi).
     */
    template<class T>
    constexpr T princarg(T x) { return std::remainder( x, T(2*M_PI));}
//    double princarg( double ang );

    /**
     * Floating-point division modulus: return x % y.
     */
    template<class T>
    constexpr T mod(T x, T y) { return std::remainder(x, y); }
//    double mod( double x, double y);

    template<class I, class O>
    typename std::iterator_traits<I>::value_type getAlphaNorm(I src, typename std::iterator_traits<I>::difference_type len, unsigned alpha)
    {
        using T = typename std::iterator_traits<I>::value_type;
        auto a = T{};
        switch(alpha) {
            case 0u:
                a = std::accumulate(src,src + len, T{},[](auto && acc, auto && val){return acc + bool(val);});
                break;
            case 1u:
                a = std::accumulate(src,src + len, T{},[](auto && acc, auto && val){return acc + std::abs(val);});
                break;
            case 2u:
                a = std::accumulate(src,src + len, T{},[](auto && acc, auto && val){return acc + val*val;});
                break;
            default:
                a = std::accumulate(src,src + len, T{},[alpha](auto && acc, auto && val){return acc + std::pow(val,alpha);});
                break;
        }
        return alpha ? std::pow(a, T{1}/alpha) : a;
    }
    template<class I>
    typename std::iterator_traits<I>::value_type powMeanPow(I src, typename std::iterator_traits<I>::difference_type len, typename std::iterator_traits<I>::value_type  alpha)
    {
        using T = typename std::iterator_traits<I>::value_type;
        auto a = T{};
        if(alpha == 0.0f)
            a = std::accumulate(src,src + len, T{},[](auto && acc, auto && val){return acc + bool(val);});
        else if(alpha == 1.0f)
            a = std::accumulate(src,src + len, T{},[](auto && acc, auto && val){return acc + std::abs(val);});
        else if(alpha == 2.0f)
            a = std::accumulate(src,src + len, T{},[](auto && acc, auto && val){return acc + val*val;});
        else
            a = std::accumulate(src,src + len, T{},[alpha](auto && acc, auto && val){return acc + std::pow(val,alpha);});
        return alpha ? std::pow(a / len, T{1}/alpha) : a/len;
    }

//    double getAlphaNorm(const double *data, unsigned int len, unsigned int alpha, double* ANorm);
//    double getAlphaNorm(const std::vector <double> &data, unsigned int alpha );

//    void   circShift( double* data, int length, int shift);
    template<class I>
    void circShift(I data, typename std::iterator_traits<I>::difference_type length, typename std::iterator_traits<I>::difference_type shift)
    {
        std::rotate(data,data + length - shift, data + length);
    }
//    int   getMax( double* data, unsigned int length, double* max = 0 );
//    int   getMax( const std::vector<double> &data, double* max = 0 );
    int    compareInt(const void * a, const void * b);

    enum NormaliseType {
        NormaliseNone,
        NormaliseUnitSum,
        NormaliseUnitMax
    };

    void normalise(double *data, int length,
                          NormaliseType n = NormaliseUnitMax);

    void normalise(std::vector<double> &data,
                          NormaliseType n = NormaliseUnitMax);

    template<class C>
    void adaptiveThreshold(C &data)
    {
        using T = typename C::value_type;
        int sz = int(data.size());
        if (sz == 0)
            return;

        std::vector<T> smoothed(sz);

        auto p_pre = 8;
        auto p_post = 7;

        for (auto i = 0; i < sz; ++i) {
            auto first = std::max(0,      i - p_pre);
            auto last  = std::min(sz - 1, i + p_post);

            smoothed[i] = mean(data, first, last - first + 1);
        }
        for (auto i = 0; i < sz; i++) {
            data[i] -= smoothed[i];
            if (data[i] < 0.0)
                data[i] = 0.0;
        }
    }
#if 0
    /**
     * Threshold the input/output vector data against a moving-mean
     * average filter.
     */
    template<class C>
    void adaptiveThreshold(C &data)
    {
        using T = typename C::value_type;
        int sz = int(data.size());
        if (sz == 0)
            return;
        std::array<T,16> sbuf{};
        auto mask = 15;
        auto sidx = 0;
        auto it = std::begin(data),ib = it, et = std::end(data);
        auto acc = T{};
        auto div = T{};
        for(auto stp = ib + std::min(8,sz); it != stp;) {
            sbuf[sidx&mask] = *it++;
            acc += sbuf[(sidx++)&mask];div += 1.0f;
        }
        if(it == et) {
            std::transform(ib,et,ib,[val=(acc/sidx)](auto && x){return std::max(0.0f,x-val);});
            return;
        }
        for(auto stp = ib + std::min(16,sz); it != stp;) {
            acc += (sbuf[sidx++&mask] = *it++);
            div += 1.0f;
            *ib = std::max(0.0f,*ib - (acc / div));
            ++ib;
        }
        for(; it != et; ++it) {
            acc -=  sbuf[sidx    &mask];
            acc += (sbuf[(sidx++)&mask] = *it++);
            *ib = std::max(0.0f, *ib - (acc /div));
            ++ib;
        }
        for(; ib != et; ++ib) {
            acc -= sbuf[sidx++&mask];
            div -= 1.0f;
            *ib = std::max(0.0f, *ib - (acc/div));
            ++ib;
        }
    }
#endif
#if 0
    template<class C>
    void adaptiveThreshold(C &data)
    {
        using T = typename C::value_type;
        if(data.empty())
            return;
        int p_pre = 8;
        int p_post = 7;

        auto smoothed = std::vector<T>(data.size() + p_pre);
        auto s_mid = std::begin(smoothed) + p_pre;
        std::copy(std::begin(data),std::end(data),s_mid);
        std::partial_sum(s_mid, std::end(smoothed),s_mid);
        auto s_mov = s_mid;
        auto s_end = std::end(smoothed);
        auto s_beg = std::begin(smoothed);
        auto acc = 0.f;
        for(; s_mov - s_mid < p_post && s_mov != s_end;)
            acc += *s_mov++;
        for(;s_beg != s_mid && s_mov != s_end;) {
            acc += *s_mov++;
            *s_beg++ = acc / (s_mov - s_mid);
        }
        for(;s_mov != s_end;) {
            acc += *s_mov++ - *s_beg;
            *s_beg = acc / ( s_mov - s_beg);
            ++s_beg;
        }
        for(;s_beg != s_mov;) {
            acc -= *s_beg;
            *s_beg = acc / (s_mov - s_beg);
            ++s_beg;
        }
        auto hwr = [](auto && x){return T(0.5) * (x + std::abs(x));};
        std::transform(std::begin(data),std::end(data),std::begin(smoothed),std::begin(data),
            [=](auto && d, auto && s){return hwr(d - s);});
/*        auto acc = std::accumulate(s_mid,s_mid +
        for (int i = 0; i < sz; ++i) {

            int first = std::max(0,      i - p_pre);
            int last  = std::min(sz - 1, i + p_post);

            smoothed[i] = mean(data, first, last - first + 1);
        }

        for (int i = 0; i < sz; i++) {
            data[i] -= smoothed[i];
            if (data[i] < 0.0)
                data[i] = 0.0;
        }*/
    }
#endif
    /**
     * Return true if x is 2^n for some integer n >= 0.
     */
    template<class T>
    constexpr int _digits() { return std::numeric_limits<T>::digits;}
    template<class T>
    constexpr T roundup(T x)
    {
        using U = std::make_unsigned_t<T>;
        auto u = U(x-U(1));
        for(auto i = 1ul; i < (_digits<U>() >>1); i <<= 1)
            u |= (u>>i);
        return T(u + U(1));
    }
    namespace builtin {
        template<class T> struct _clz {};
        template<> struct _clz<uint32_t> {
            constexpr int operator()(uint32_t t) { return __builtin_clz(t);}
            using restype = int;
        };
        template<> struct _clz<int32_t> {
            constexpr int operator()(int32_t t) { return __builtin_clz(t);}
            using restype = int;
        };
        template<> struct _clz<int64_t> {
            constexpr int operator()(int64_t t) { return __builtin_clzl(t);}
            using restype = int;
        };
        template<> struct _clz<uint64_t> {
            constexpr int operator()(uint64_t t) { return __builtin_clzl(t);}
            using restype = int;
        };
        template<> struct _clz<uint16_t> {
            constexpr int operator()(uint16_t t) { return __builtin_clz(uint32_t(t));}
            using restype = int;
        };
        template<> struct _clz<int16_t> {
            constexpr int operator()(int16_t t) { return __builtin_clz(uint32_t(t));}
            using restype = int;
        };
    }
    template<class T>
    constexpr typename builtin::_clz<T>::restype clz(T t)
    {
        return builtin::_clz<T>{}(t);
    }
    template<class T>
    constexpr typename builtin::_clz<T>::restype ilog2(T x)
    {
        return _digits<T>() + 1 - clz(x);
    }
    template<class T>
    constexpr std::enable_if_t<
        std::is_integral<T>::value
      , bool
        > is_pow_2(T x)
    {
        return (x > T{0}) && !(x&(x-T(1)));
    }
    template<class T>
    constexpr std::enable_if_t<
        std::is_integral<T>::value
      , bool
        > isPowerOfTwo(T x)
    {
        return is_pow_2(x);
    }
    template<class T, T n>
    struct is_power_of_two : std::integral_constant<bool, is_pow_2(n)> {};

    template<class T, T n>
    constexpr bool is_power_of_two_v = is_power_of_two<T,n>::value;


    /**
     * Return the next higher integer power of two from x, e.g. 1300
     * -> 2048, 2048 -> 2048.
     */
    template<class T>
    constexpr std::enable_if_t<
        std::is_integral<T>::value
      , bool
        > nextPowerOfTwo(T x)
    {
        return T{1} << ilog2(x-T{1});
    }
    template<class T>
    constexpr std::enable_if_t<
        std::is_integral<T>::value
      , bool
        > previousPowerOfTwo(T x)
    {
        return T{1} << (ilog2(x)-1);
    }
    /**
     * Return the next lower integer power of two from x, e.g. 1300 ->
     * 1024, 2048 -> 2048.
     */

    /**
     * Return the nearest integer power of two to x, e.g. 1300 -> 1024,
     * 12 -> 16 (not 8; if two are equidistant, the higher is returned).
     */
    template<class T>
    constexpr std::enable_if_t<
        std::is_integral<T>::value
      , bool
        > nearestPowerOfTwo(T x)
    {
        auto _next = nextPowerOfTwo(x);
        auto _prev = previousPowerOfTwo(x);
        return std::abs(_next - x) <= std::abs(x - _prev) ? _next : _prev;
    }

    /**
     * Return x!
     */
    double factorial(int x); // returns double in case it is large

    /**
     * Return the greatest common divisor of natural numbers a and b.
     */
    template<class T>
    constexpr std::enable_if_t<
        std::is_integral<T>::value
      , T
        > gcd(T a, T b)
    {
        std::tie(b,a) = std::minmax(a,b);
        while(b) {
            auto c = a % b;
            a = b;
            b = c;
        }
        return a;
//        auto c = a % b;
    }
};

#endif
