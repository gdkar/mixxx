/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2014 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#ifndef _RUBBERBAND_VECTOR_OPS_COMPLEX_H_
#define _RUBBERBAND_VECTOR_OPS_COMPLEX_H_

#include "VectorOps.h"
#include "base/Profiler.h"
#include <cmath>
namespace RubberBand {


template<typename T>
inline void c_phasor(T &real, T &imag, T phase)
{
    //!!! IPP contains ippsSinCos_xxx in ippvm.h -- these are
    //!!! fixed-accuracy, test and compare
#if defined HAVE_VDSP
    int one = 1;
    if (sizeof(T) == sizeof(float)) {
        vvsincosf((float *)&imag, (float *)&real, (const float *)&phase, &one);
    } else {
        vvsincos((double *)&imag, (double *)&real, (const double *)&phase, &one);
    }
#elif defined LACK_SINCOS
        real = std::cos(phase);
        imag = std::sin(phase);
#elif defined __GNUC__
    if (sizeof(T) == sizeof(float)) {
        sincosf(phase, (float *)&imag, (float *)&real);
    } else {
        sincos(phase, (double *)&imag, (double *)&real);
    }
#else
        *real = std::cos(phase);
        *imag = std::sin(phase);
#endif
}
template<typename T>
inline T approximate_atan2(T x, T y){
    static const T coeff1 = M_PI/4;
    static const T coeff2 = 3*M_PI/4;
    const T  absy=(y<0)?-y:y;
    const T r = (x>=0)?((x-absy)/(x+absy)):((x+absy)/(absy-x));
    const T r2 = r*r;
    const T a = (0.1963f*r2 - 0.9817)*r +( (x>=0)?(coeff1):(coeff2));
    return (y<0)?-a:a;
}
#ifndef USE_APPROXIMATE_ATAN2
template<typename T>
inline void c_magphase(T &mag, T &phase, T real, T imag)
{
    mag = std::sqrt(real * real + imag * imag);
    phase = std::atan2(imag, real);
}
#else
template<typename T >
inline void c_magphase(T &mag, T &phase, T real, T imag)
{
    T atan = approximate_atan2<T>(real, imag);
    phase = atan;
    mag = std::sqrt(real * real + imag * imag);
}

// NB arguments in opposite order from usual for atan2f

#endif


template<typename S, typename T> // S source, T target
void v_polar_to_cartesian(T *const R__ real,
                          T *const R__ imag,
                          const S *const R__ mag,
                          const S *const R__ phase,
                          const int count)
{
    Profiler profiler("VectorOpsComplex::v_polar_to_cartesian");
    (void)__builtin_assume_aligned(real,16);
    (void)__builtin_assume_aligned(imag,16);
    (void)__builtin_assume_aligned(mag,16);
    (void)__builtin_assume_aligned(phase,16);
    for (int i = 0; i < count; ++i) {
        c_phasor<T>(real[i], imag[i], phase[i]);
        const T magi =T( mag[i]);
        real[i]*=magi;
        imag[i]*=magi;
    }
}

template<typename T>
void v_polar_interleaved_to_cartesian_inplace(T *const R__ srcdst,
                                              const int count)
{
    Profiler profiler("VectorOpsComplex::v_polar_interleaved_to_cartesian_inplace");
    for (int i = 0; i < count*2; i += 2) {
        T real, imag;
        c_phasor(real, imag, srcdst[i+1]);
        real *= srcdst[i];
        imag *= srcdst[i];
        srcdst[i] = real;
        srcdst[i+1] = imag;
    }
}

template<typename S, typename T> // S source, T target
void v_polar_to_cartesian_interleaved(T *const R__ dst,
                                      const S *const R__ mag,
                                      const S *const R__ phase,
                                      const int count)
{
    Profiler profiler("VectorOpsComplex::v_polar_to_cartesian_interleaved");
    for (int i = 0; i < count; ++i) {
        T real, imag;
        c_phasor<T>(real,imag, phase[i]);
        real *= mag[i];
        imag *= mag[i];
        dst[i*2] = real;
        dst[i*2+1] = imag;
    }
}    

#if defined USE_POMMIER_MATHFUN
void v_polar_to_cartesian_pommier(float *const R__ real,
                                  float *const R__ imag,
                                  const float *const R__ mag,
                                  const float *const R__ phase,
                                  const int count);
void v_polar_interleaved_to_cartesian_inplace_pommier(float *const R__ srcdst,
                                                      const int count);
void v_polar_to_cartesian_interleaved_pommier(float *const R__ dst,
                                              const float *const R__ mag,
                                              const float *const R__ phase,
                                              const int count);

template<>
inline void v_polar_to_cartesian(float *const R__ real,
                                 float *const R__ imag,
                                 const float *const R__ mag,
                                 const float *const R__ phase,
                                 const int count)
{
    v_polar_to_cartesian_pommier(real, imag, mag, phase, count);
}

template<>
inline void v_polar_interleaved_to_cartesian_inplace(float *const R__ srcdst,
                                                     const int count)
{
    v_polar_interleaved_to_cartesian_inplace_pommier(srcdst, count);
}

template<>
inline void v_polar_to_cartesian_interleaved(float *const R__ dst,
                                             const float *const R__ mag,
                                             const float *const R__ phase,
                                             const int count)
{
    v_polar_to_cartesian_interleaved_pommier(dst, mag, phase, count);
}

#endif

template<typename S, typename T> // S source, T target
void v_cartesian_to_polar(T *const R__ mag,
                          T *const R__ phase,
                          const S *const R__ real,
                          const S *const R__ imag,
                          const int count)
{
    Profiler profiler("VectorOpsComplex::v_cartesian_to_polar");
    for (int i = 0; i < count; ++i) {
        c_magphase<T>(mag[i], phase[i], real[i], imag[i]);
    }
}

template<typename S, typename T> // S source, T target
void v_cartesian_interleaved_to_polar(T *const R__ mag,
                                      T *const R__ phase,
                                      const S *const R__ src,
                                      const int count)
{
    Profiler profiler("VectorOpsComplex::v_cartesian_interleaved_to_polar");
    for (int i = 0; i < count; ++i) {
        c_magphase<T>(mag[i], phase[i], src[i*2], src[i*2+1]);
    }
}

#ifdef HAVE_VDSP
template<>
inline void v_cartesian_to_polar(float *const R__ mag,
                                 float *const R__ phase,
                                 const float *const R__ real,
                                 const float *const R__ imag,
                                 const int count)
{
    DSPSplitComplex c;
    c.realp = const_cast<float *>(real);
    c.imagp = const_cast<float *>(imag);
    vDSP_zvmags(&c, 1, phase, 1, count); // using phase as a temporary dest
    vvsqrtf(mag, phase, &count); // using phase as the source
    vvatan2f(phase, imag, real, &count);
}
template<>
inline void v_cartesian_to_polar(double *const R__ mag,
                                 double *const R__ phase,
                                 const double *const R__ real,
                                 const double *const R__ imag,
                                 const int count)
{
    // double precision, this is significantly faster than using vDSP_polar
    DSPDoubleSplitComplex c;
    c.realp = const_cast<double *>(real);
    c.imagp = const_cast<double *>(imag);
    vDSP_zvmagsD(&c, 1, phase, 1, count); // using phase as a temporary dest
    vvsqrt(mag, phase, &count); // using phase as the source
    vvatan2(phase, imag, real, &count);
}
#endif

template<typename T>
void v_cartesian_to_polar_interleaved_inplace(T *const R__ srcdst,
                                              const int count)
{
    Profiler profiler("VectorOpsComplex::v_cartesian_to_polar_interleaved_inplace");
    for (int i = 0; i < count * 2; i += 2) {
        T mag, phase;
        c_magphase(mag, phase, srcdst[i], srcdst[i+1]);
        srcdst[i] = mag;
        srcdst[i+1] = phase;
    }
}

}

#endif

