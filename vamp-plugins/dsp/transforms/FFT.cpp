/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
*/

#include "FFT.h"

#include <fftw3.h>
#include "maths/MathUtilities.h"

#include <cmath>

#include <iostream>
#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <numeric>
#include <stdexcept>
namespace  {
template<class T>
struct fftwf_deleter {
    void operator() ( T * ptr)
    {
        fftwf_free(ptr);
    }
};
}
class FFT::D
{
public:
    D(int n)
    : m_n(n)
    , m_freal_in((float*)fftw_malloc( n * sizeof(float)))
    , m_fimag_in((float*)fftw_malloc( n * sizeof(float)))
    , m_freal_out((float*)fftw_malloc( n * sizeof(float)))
    , m_fimag_out((float*)fftw_malloc( n * sizeof(float)))
    {
        auto dims= fftw_iodim{ n, 1, 1 };
        m_plan = fftwf_plan_guru_split_dft(1, &dims, 0, nullptr, m_freal_in.get(), m_fimag_in.get(), m_freal_out.get(), m_fimag_out.get(), FFTW_MEASURE);
    }

    ~D() {
        fftwf_destroy_plan(m_plan);
    }

    void process(bool inverse,
                 const double *ri,
                 const double *ii,
                 double *ro,
                 double *io) {
        if(inverse) {
            std::swap(ri, ii);
            std::swap(ro, io);
        }
        if(ri)
            std::copy_n(ri, m_n, m_freal_in.get());
        else
            std::fill_n(m_freal_in.get(), m_n, 0.f);
        if(ii)
            std::copy_n(ii, m_n, m_fimag_in.get());
        else
            std::fill_n(m_fimag_in.get(), m_n, 0.f);
        fftwf_execute(m_plan);
        if(inverse) {
            auto factor = 1.0f / m_n;
            if(ro)
                std::transform(m_freal_out.get(), m_freal_out.get() + m_n, ro,[factor](auto x){return x * factor;});
            if(io)
                std::transform(m_fimag_out.get(), m_fimag_out.get() + m_n, io,[factor](auto x){return x * factor;});

        } else {
            if(ro)
                std::copy_n(m_freal_out.get(), m_n, ro);

            if(io)
                std::copy_n(m_fimag_out.get(), m_n, io);
        }
    }

private:
    int m_n;
    std::unique_ptr<float[],fftwf_deleter<float>> m_freal_in;
    std::unique_ptr<float[],fftwf_deleter<float>> m_fimag_in;
    std::unique_ptr<float[],fftwf_deleter<float>> m_freal_out;
    std::unique_ptr<float[],fftwf_deleter<float>> m_fimag_out;

    fftwf_plan   m_plan;
};

FFT::FFT(int n) :
    m_d(new D(n))
{
}

FFT::~FFT()
{
    delete m_d;
}

void
FFT::process(bool inverse,
             const double *p_lpRealIn, const double *p_lpImagIn,
             double *p_lpRealOut, double *p_lpImagOut)
{
    m_d->process(inverse,
                 p_lpRealIn, p_lpImagIn,
                 p_lpRealOut, p_lpImagOut);
}

class FFTReal::D
{
public:
    D(int n)
    : m_n(n)
    , m_c(n / 2 + 1)
    , m_fbuf((float*)fftw_malloc( n * sizeof(float)))
    , m_fimag((float*)fftw_malloc( m_c * sizeof(float)))
    , m_freal((float*)fftw_malloc( m_c * sizeof(float)))
    {
        auto dims= fftw_iodim{ n, 1, 1 };
        m_planf = fftwf_plan_guru_split_dft_r2c(1, &dims, 0, nullptr, m_fbuf.get(), m_freal.get(), m_fimag.get(), FFTW_MEASURE);
        m_plani = fftwf_plan_guru_split_dft_c2r(1, &dims, 0, nullptr, m_freal.get(), m_fimag.get(), m_fbuf.get(), FFTW_MEASURE);
    }

    ~D() {
        fftwf_destroy_plan(m_planf);
        fftwf_destroy_plan(m_plani);
    }

    void forward(const double *ri, double *ro, double *io) {
        std::copy_n(ri, m_n, m_fbuf.get());
        fftwf_execute(m_planf);
        std::reverse_copy(m_freal.get(), m_freal.get() + m_c - 1, std::copy(m_freal.get(), m_freal.get() + m_c, ro));
        std::reverse_copy(m_fimag.get(), m_fimag.get() + m_c - 1, std::copy(m_fimag.get(), m_fimag.get() + m_c, io));
        std::transform(io + m_c, io + m_n, io + m_c, [](auto x){return -x;});
    }

    void forwardMagnitude(const double *ri, double *mo) {

        std::copy_n(ri, m_n, m_fbuf.get());
        fftwf_execute(m_planf);
        std::transform(m_freal.get(), m_freal.get() + m_c, m_fimag.get(), mo, [](auto x, auto y){return std::hypot(x,y);});
        std::reverse_copy(mo, mo + m_c - 1, mo + m_c);
    }

    void inverse(const double *ri, const double *ii, double *ro) {
        std::copy_n(ri, m_c, m_freal.get());
        std::copy_n(ii, m_c, m_fimag.get());
        fftwf_execute(m_plani);
        auto scale = 1.0f / m_n;
        std::transform(m_fbuf.get(),m_fbuf.get() + m_n, ro, [scale](auto x){return x * scale;});
    }

private:
    int m_n;
    int m_c{m_n / 2 + 1};
    std::unique_ptr<float[],fftwf_deleter<float>> m_fbuf;
    std::unique_ptr<float[],fftwf_deleter<float>> m_freal;
    std::unique_ptr<float[],fftwf_deleter<float>> m_fimag;

    fftwf_plan   m_planf;
    fftwf_plan   m_plani;
};

FFTReal::FFTReal(int n) :
    m_d(new D(n))
{
}

FFTReal::~FFTReal()
{
    delete m_d;
}

void
FFTReal::forward(const double *ri, double *ro, double *io)
{
    m_d->forward(ri, ro, io);
}

void
FFTReal::forwardMagnitude(const double *ri, double *mo)
{
    m_d->forwardMagnitude(ri, mo);
}

void
FFTReal::inverse(const double *ri, const double *ii, double *ro)
{
    m_d->inverse(ri, ii, ro);
}
