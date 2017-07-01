#pragma once
#include <cmath>
#include <cstdlib>
#include <map>
#include "rubberband/system/Math.hpp"
#include "rubberband/system/sysutils.hpp"
#include "rubberband/system/Simd.hpp"
#include "rubberband/system/VectorOps.hpp"
#include "rubberband/system/Allocators.hpp"

namespace {
constexpr const float A[] =
{
-1.30002500998624804212E-8f,
 6.04699502254191894932E-8f,
-2.67079385394061173391E-7f,
 1.11738753912010371815E-6f,
-4.41673835845875056359E-6f,
 1.64484480707288970893E-5f,
-5.75419501008210370398E-5f,
 1.88502885095841655729E-4f,
-5.76375574538582365885E-4f,
 1.63947561694133579842E-3f,
-4.32430999505057594430E-3f,
 1.05464603945949983183E-2f,
-2.37374148058994688156E-2f,
 4.93052842396707084878E-2f,
-9.49010970480476444210E-2f,
 1.71620901522208775349E-1f,
-3.04682672343198398683E-1f,
 6.76795274409476084995E-1f
};
constexpr const float B[] =
{
 3.39623202570838634515E-9f,
 2.26666899049817806459E-8f,
 2.04891858946906374183E-7f,
 2.89137052083475648297E-6f,
 6.88975834691682398426E-5f,
 3.36911647825569408990E-3f,
 8.04490411014108831608E-1f
};
float constexpr chbevlf( float x, const float *array, int n )
{
    auto p = array;
    auto b0 = *p++;
    auto b1 = 0.0f;
    auto i = n - 1;
    auto b2 = 0.f;

    do {
        b2 = b1;
        b1 = b0;
        b0 = x * b1  -  b2  + *p++;
    } while( --i );

    return( 0.5f*(b0-b2) );
}

constexpr const float A1[] =
{
 9.38153738649577178388E-9f,
-4.44505912879632808065E-8f,
 2.00329475355213526229E-7f,
-8.56872026469545474066E-7f,
 3.47025130813767847674E-6f,
-1.32731636560394358279E-5f,
 4.78156510755005422638E-5f,
-1.61760815825896745588E-4f,
 5.12285956168575772895E-4f,
-1.51357245063125314899E-3f,
 4.15642294431288815669E-3f,
-1.05640848946261981558E-2f,
 2.47264490306265168283E-2f,
-5.29459812080949914269E-2f,
 1.02643658689847095384E-1f,
-1.76416518357834055153E-1f,
 2.52587186443633654823E-1f
};
constexpr const float B1[] =
{
-3.83538038596423702205E-9f,
-2.63146884688951950684E-8f,
-2.51223623787020892529E-7f,
-3.88256480887769039346E-6f,
-1.10588938762623716291E-4f,
-9.76109749136146840777E-3f,
 7.78576235018280120474E-1f
};

constexpr float i0f( float x )
{
    x = std::abs(x);
    if( x <= 8.0f ) {
        auto y = 0.5f*x - 2.0f;
        return( std::exp(x) * chbevlf( y, A, std::extent<decltype(A)>::value ) );
	}
    return(  std::exp(x) * chbevlf( 32.0f/x - 2.0f, B, std::extent<decltype(B)>::value ) / std::sqrt(x) );
}
constexpr float i1f(float xx)
{

    auto x = xx;
    auto z = std::abs(xx);
    if( z <= 8.0f ) {
        auto y = 0.5f*z - 2.0f;
        z = chbevlf( y, A1, 17 ) * z * std::exp(z);
    } else {
        z = std::exp(z) * chbevlf( 32.0f/z - 2.0f, B1, 7 ) / std::sqrt(z);
    }
    if( x < 0.0f )
        z = -z;
    return( z );
}

}

namespace RBMixxxVamp {
template<class It>
It make_kaiser_window(It _beg, It _end, float alpha)
{
    auto oneOverDenom = 1.0f / i0f(alpha);
    auto N = std::distance(_beg,_end) - 1;
    auto twoOverN = 2.0f / N;
    auto n = 0;
    for(; _beg != _end; ++n) {
        auto K = n * twoOverN - 1;
        auto arg = (bs::sqrt( 1.0f - bs::sqr(K))) * alpha;
        *_beg++ = i0f(arg) * oneOverDenom;
	}
    return _beg;
}

template<class It>
It make_kaiser_bessel_derived_window(It _beg, It _end, float alpha)
{
    auto N = std::distance(_beg,_end);
    auto _mid = _beg + (N/2);
    make_kaiser_window(_beg,_mid + 1,alpha);
    std::partial_sum(_beg,_mid + 1,_beg);
    auto _norm = 1 / *_mid;

    bs::transform(_beg,_mid,_beg,[_norm](auto x){return bs::sqrt(x * _norm);});
    return std::reverse_copy(_beg, _mid, _mid);
}
template<class It>
It make_xiph_vorbis_window(It _beg, It _end)
{
    using T = typename std::iterator_traits<It>::value_type;
    constexpr auto half_pi = T_PI_2<T>;

    auto N = std::distance(_beg, _end);
    auto pi_over_N =  T_PI<T> / N;
    std::iota(_beg,_end, T{0.5});

    return bs::transform(_beg,_end,_beg,[pi_over_N,half_pi](auto x) {
        return bs::sin(half_pi * bs::sqr(bs::sin(x * pi_over_N)));
    });
}

template<class It, class S>
It make_sinc_window(It _beg, It _end,S p)
{
    using T = typename std::iterator_traits<It>::value_type;
    auto N = std::distance(_beg,_end);
    auto half = N/2;
    auto _pos = _beg + half;
    auto _neg = _pos;
    *_pos++ = T{1};
    auto a = bs::Twopi<T>() / p;
    auto inc = a;
    for(;_pos != _end; a += inc) {
        auto v = bs::sinc(a);
        *_pos++ = v;
        *--_neg = v;
    }
}
}
