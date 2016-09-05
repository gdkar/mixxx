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
#include <algorithm>
#include <utility>
#include <type_traits>
#include <functional>
#include <memory>
#include <cmath>
#include "nan-inf.h"

/**
 * Static helper functions for simple mathematical calculations.
 */
namespace MathUtilities
{
    /**
     * Return the mean of the given array of the given length.
     */
    template<class T>
    T mean(const T *src, unsigned int len)
    {
        if (len == 0) return 0;
        return std::accumulate(src,src+len,T(0)) / T(len);
    }
    template<class T>
    T mean(const std::vector<T> &src, unsigned int start, unsigned int count)
    {
        return mean(&src[start],count);
    }
    template<class T>
    constexpr T mod(T x, T y)
    {
        auto a = std::floor( x / y );
        return  x - ( y * a );
    }
    template<class T>
    constexpr T princarg(T ang)
    {
        return mod( ang + T(M_PI), T(-2 * M_PI) ) + T(M_PI);
    }
    template<class T>
    void getAlphaNorm(const T *data, unsigned int len, unsigned int alpha, T * ANorm)
    {
        unsigned int i;
        T temp = 0.0;
        T a=0.0;

        for( i = 0; i < len; i++)
        {
            temp = data[ i ];

            a  += std::pow( std::abs(temp), T(alpha) );
        }
        a /= ( T )len;
        a = std::pow( a, ( 1.0f / (T) alpha ) );
        *ANorm = a;
    }
    template<class T>
    T getAlphaNorm( const std::vector <T > &data, unsigned int alpha )
    {
        unsigned int i;
        unsigned int len = data.size();
        T temp = 0.0;
        T a=0.0;

        for( i = 0; i < len; i++)
        {
            temp = data[ i ];
            a  += std::pow( std::abs(temp), T(alpha) );
        }
        a /= ( T)len;
        return std::pow( a, ( T(1) / (T) alpha ) );
    }
    template<class T>
    constexpr T round(T x)
    {
        return std::round(x);
    }
    template<class T>
    T median(const T *src, unsigned int len)
    {
        if (len == 0)
            return 0;
        auto scratch = std::vector<T>{src,src+len};
        int middle = len/2;
        std::nth_element(scratch.begin(),std::next(scratch.begin(),middle),scratch.end());
        if(len & 1)
            return scratch[middle];
        auto next = *std::min_element(std::next(scratch.begin(),middle+1),scratch.end());
        return (scratch[middle] + next) * T(0.5);
    }
    template<class T>
    void circShift(T *data, int len, int shift)
    {
        std::rotate(data, data + (shift %len), data + len);
    }
    inline int compareInt(const void * a, const void * b)
    {
        return ( *(int*)a - *(int*)b );
    }
    enum NormaliseType {
        NormaliseNone,
        NormaliseUnitSum,
        NormaliseUnitMax
    };
    template<class T>
    void normalise(T *data, int length, NormaliseType type)
    {
        switch (type) {

        case NormaliseNone: return;

        case NormaliseUnitSum:
        {
            T sum = 0.0;
            for (int i = 0; i < length; ++i) {
                sum += data[i];
            }
            if (sum != 0.0) {
                for (int i = 0; i < length; ++i) {
                    data[i] /= sum;
                }
            }
        }
        break;

        case NormaliseUnitMax:
        {
            T max = 0.0;
            for (int i = 0; i < length; ++i) {
                if (fabs(data[i]) > max) {
                    max = fabs(data[i]);
                }
            }
            if (max != 0.0) {
                for (int i = 0; i < length; ++i) {
                    data[i] /= max;
                }
            }
        }
        break;

        }
    }
    template<class T>
    void normalise(std::vector<T> &data, NormaliseType type)
    {
        switch (type) {

        case NormaliseNone: return;

        case NormaliseUnitSum:
        {
            T sum = 0.0;
            for (int i = 0; i < (int)data.size(); ++i) sum += data[i];
            if (sum != 0.0) {
                for (int i = 0; i < (int)data.size(); ++i) data[i] /= sum;
            }
        }
        break;

        case NormaliseUnitMax:
        {
            T max = 0.0;
            for (int i = 0; i < (int)data.size(); ++i) {
                if (fabs(data[i]) > max) max = fabs(data[i]);
            }
            if (max != 0.0) {
                for (int i = 0; i < (int)data.size(); ++i) data[i] /= max;
            }
        }
        break;

        }
    }

    /**
     * Threshold the input/output vector data against a moving-mean
     * average filter.
     */

    template<class T>
    void adaptiveThreshold(std::vector<T> &data)
    {
        int sz = int(data.size());
        if (sz == 0) return;

        std::vector<T> smoothed(sz);

        int p_pre = 8;
        int p_post = 7;

        for (int i = 0; i < sz; ++i) {

            int first = std::max(0,      i - p_pre);
            int last  = std::min(sz - 1, i + p_post);

            smoothed[i] = mean(data, first, last - first + 1);
        }

        for (int i = 0; i < sz; i++) {
            data[i] -= smoothed[i];
            if (data[i] < 0.0) data[i] = 0.0;
        }
    }
    /**
     * Return true if x is 2^n for some integer n >= 0.
    */
    template<class T>
    constexpr std::enable_if_t<std::is_integral<T>::value,bool>
    isPowerOfTwo(T x)
    {
        return x && !(x &(x-1));
    }
    /**
     * Return the next higher integer power of two from x, e.g. 1300
     * -> 2048, 2048 -> 2048.
     */
    template<class T>
    constexpr std::enable_if_t<std::is_integral<T>::value,T>
    nextPowerOfTwo(T x)
    {
        using U = std::make_unsigned_t<T>;
        auto u = U(x) - U(1);
        for(auto i = 1; i < (std::numeric_limits<U>::digits / 2); i <<= 1)
            u |= (u >> i);
        auto t = T(u + U(1));
        return t + (!t);
    }
    template<class T>
    constexpr std::enable_if_t<std::is_integral<T>::value,T>
    previousPowerOfTwo(T x)
    {
        if(x < T(1)) return T(1);
        if(isPowerOfTwo(x)) return x;
        return (nextPowerOfTwo(x) >> 1);
    }
    template<class T>
    constexpr std::enable_if_t<std::is_integral<T>::value,T>
    nearestPowerOfTwo(T x)
    {
        if(isPowerOfTwo(x)) return x;
        auto n0 = previousPowerOfTwo(x),
             n1 = nextPowerOfTwo(x);
        return (x - n0 < n1 - x) ? n0 : n1;
    }
    constexpr double factorial(int x)
    {
        auto f = 1.;
        for(auto i = 1., dx = double(x); i <= x; i += 1.)
            f *= i;
        return f;
    }
    template<class T>
    constexpr std::enable_if_t<std::is_integral<T>::value,T>
    gcd(T a, T b)
    {
        if(auto c = a % b) {
            return gcd(b,c);
        }else{
            return b;
        }
    }
};

#endif
