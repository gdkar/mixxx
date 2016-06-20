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
#include <complex>
#include <limits>
#include <functional>
#include <tuple>
#include <climits>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <iostream>
#include <type_traits>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <utility>
#include <cmath>

/**
 * helper functions for simple mathematical calculations.
 */
namespace MathUtilities  
{
    /**
     * Return the sum of the values in the given array of the given
     * length.
     */
    template<class Iter>
    typename std::iterator_traits<Iter>::value_type  sum(Iter src, unsigned int len)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        return std::accumulate(src,std::next(src,len),value_type(0));
    }
    template<class Iter>
    typename std::iterator_traits<Iter>::value_type sum(Iter start, Iter stop)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        return std::accumulate(start,stop,value_type{0});
    }
    template<class Iter>
    typename std::iterator_traits<Iter>::value_type mean(Iter start, Iter stop)
    {
        return sum(start,stop) / std::distance(start,stop);
    }
    template<class Iter>
    typename std::iterator_traits<Iter>::value_type mean(Iter src, unsigned int len)
    {
        return mean(&src[0],&src[len]);
    }
    /**
     * Return the median of the values in the given array of the given
     * length. If the array is even in length, the returned value will
     * be half-way between the two values adjacent to median.
     */
    template<class Iter>
    typename std::iterator_traits<Iter>::value_type median(Iter start, Iter stop)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        auto  tmp = std::vector<value_type>(start,stop);
        auto mid = std::next(tmp.begin(),tmp.size()/2);
        std::nth_element(tmp.begin(), mid,tmp.end());
        return *mid;
    }
    /**
     * Floating-point division modulus: return x % y.
     */
    template<class T>
    constexpr T mod(T x, T y) { return x - (std::floor(x/y) * y);}
    template<class T>
    constexpr T princarg(T ang)
    {
        return mod<T>(ang + T(M_PI), T(-2*M_PI)) + T(M_PI);
    }
    template<class Iter>
    typename std::iterator_traits<Iter>::value_type getAlphaNorm(Iter start, Iter stop, unsigned int alpha)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        return std::pow(std::accumulate(start,stop,value_type(0),[=](auto lhs, auto rhs){
            return lhs + std::pow(std::abs(rhs),alpha);
            }) / std::distance(start,stop), value_type(1) / value_type(alpha));

    }
    template<class Iter>
    typename std::iterator_traits<Iter>::value_type getAlphaNorm(Iter start, unsigned int len, unsigned int alpha)
    {
        return getAlphaNorm(start,std::next(start,len),alpha);
    }
    template<class Iter>
    auto getMax(Iter start, Iter stop)
    {
        return std::distance(start,std::max_element(start,stop));
    }
    enum NormaliseType {
        NormaliseNone,
        NormaliseUnitSum,
        NormaliseUnitMax
    };
    template<class Iter>
    void normalize(Iter start, Iter stop)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        auto normalizer = value_type(1) / std::max(std::numeric_limits<value_type>::epsilon(),std::accumulate(start,stop,value_type(0)));
        std::transform(start,stop,start,[=](const auto &x){return x * normalizer;});
    }
    template<class Iter>
    void normalize_abs(Iter start, Iter stop)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        auto normalizer = value_type(1) / std::max(std::numeric_limits<value_type>::epsilon(),
                std::abs(*std::max_element(start,stop,
                        [](const auto &x, const auto &y){return std::abs(x) < std::abs(y);}
                        )));
        std::transform(start,stop,start,[=](const auto &x){return x * normalizer;});
    }
    template<class Iter>
    void normalize_max(Iter start, Iter stop)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        auto normalizer = value_type(1) / std::max(std::numeric_limits<value_type>::epsilon(),*std::max_element(start,stop));
        std::transform(start,stop,start,[=](const auto &x){return x * normalizer;});
    }
    /** 
     * Return true if x is 2^n for some integer n >= 0.
     */
    template<class T>
    constexpr typename std::enable_if<std::is_integral<T>::value,bool>::type isPowerOfTwo(T x)
    {
        return (x && !(x & (x-1)));
    }
    template<class T>
    constexpr typename std::enable_if<std::is_integral<T>::value,T>::type 
    nextPowerOfTwo(T x)
    {
        using UT = typename std::make_unsigned<T>::type;
        auto y = static_cast<UT>(x) - UT{1};
        for(auto i = 1u; i < sizeof(UT) * 8u; i <<= 1) y |= (y>>i);
        return static_cast<T>(y + UT{1});
    }
    template<class T>
    typename std::enable_if<std::is_floating_point<T>::value,T>::type
    nextPowerOfTwo(T x)
    {
        return std::exp2(std::ceil(std::log(x)));
    }
    template<class T>
    constexpr typename std::enable_if<std::is_integral<T>::value,T>::type
    previousPowerOfTwo(T x)
    {
        return (isPowerOfTwo(x) ? x : ((nextPowerOfTwo(x) >> 1)));
    }
    template<class T>
    typename std::enable_if<std::is_floating_point<T>::value,T>::type
    previousPowerOfTwo(T x)
    {
        return std::exp2(std::floor(std::log(x)));
    }
    template<class T>
    constexpr T factorial(T x)
    {
        auto r = x;
        while(--x) r *= x;
        return r;
    }
    template<class T>
    constexpr T gcd(T a, T b)
    {
        auto c = (a > b) ? a % b : b % a;
        auto d = std::min(a,b);
        return  c ? gcd(d, c) : d;
    }
    template<class Iter>
    void adaptiveThreshold(Iter start,Iter stop)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        using difference_type = typename std::iterator_traits<Iter>::difference_type;
        auto aux = std::vector<value_type>(start,stop);
        std::partial_sum(aux.cbegin(),aux.cend(),aux.begin());
        for(auto ait = aux.cbegin(), eit = aux.cend(), it = start; ait != eit; ait++,it++) {
            auto _forward = std::next(ait,std::min(difference_type(8),std::distance(ait,eit)));
            auto _backward = std::prev(ait,std::min(difference_type(7),std::distance(aux.cbegin(),ait)));
            *it = std::max(*it - ((*_forward - *_backward) / value_type(std::distance(_backward,_forward))), value_type(0));

        }
    }

}

#endif
