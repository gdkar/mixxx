/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
*/

#include "FFT.h"

#include <fftw3.h>
#include "maths/MathUtilities.h"
//#include "ext/kissfft/kiss_fft.h"
//#include "ext/kissfft/kiss_fftr.h"

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
//        m_planf = kiss_fft_alloc(m_n, 0, NULL, NULL);
//        m_plani = kiss_fft_alloc(m_n, 1, NULL, NULL);
//        m_kin = new kiss_fft_cpx[m_n];
//        m_kout = new kiss_fft_cpx[m_n];
    }

    ~D() {
        fftwf_destroy_plan(m_plan);
//        kiss_fft_free(m_planf);
 //       kiss_fft_free(m_plani);
//        delete[] m_kin;
//        delete[] m_kout;
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
/*
        for (int i = 0; i < m_n; ++i) {
            m_kin[i].r = ri[i];
            m_kin[i].i = (ii ? ii[i] : 0.0);
        }

        if (!inverse) {

            kiss_fft(m_planf, m_kin, m_kout);

            for (int i = 0; i < m_n; ++i) {
                ro[i] = m_kout[i].r;
                io[i] = m_kout[i].i;
            }

        } else {

            kiss_fft(m_plani, m_kin, m_kout);

            double scale = 1.0 / m_n;

            for (int i = 0; i < m_n; ++i) {
                ro[i] = m_kout[i].r * scale;
                io[i] = m_kout[i].i * scale;
            }
        }*/
    }

private:
    int m_n;
    std::unique_ptr<float[],fftwf_deleter<float>> m_freal_in;
    std::unique_ptr<float[],fftwf_deleter<float>> m_fimag_in;
    std::unique_ptr<float[],fftwf_deleter<float>> m_freal_out;
    std::unique_ptr<float[],fftwf_deleter<float>> m_fimag_out;

    fftwf_plan   m_plan;
//    fftwf_plan   m_plani;
//    kiss_fft_cfg m_planf;
//    kiss_fft_cfg m_plani;
//    kiss_fft_cpx *m_kin;
//    kiss_fft_cpx *m_kout;
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
/*        auto imid = std::copy(m_fimag.get(),m_fimag.get() + m_c, io);
        kiss_fftr(m_planf, ri, m_c);

        for (int i = 0; i <= m_n/2; ++i) {
            ro[i] = m_c[i].r;
            io[i] = m_c[i].i;
        }

        for (int i = 0; i + 1 < m_n/2; ++i) {
            ro[m_n - i - 1] =  ro[i + 1];
            io[m_n - i - 1] = -io[i + 1];
        }*/
    }

    void forwardMagnitude(const double *ri, double *mo) {

        std::copy_n(ri, m_n, m_fbuf.get());
        fftwf_execute(m_planf);
        std::transform(m_freal.get(), m_freal.get() + m_c, m_fimag.get(), mo, [](auto x, auto y){return std::hypot(x,y);});
        std::reverse_copy(mo, mo + m_c - 1, mo + m_c);
/*        double *io = new double[m_n];

        forward(ri, mo, io);

        for (int i = 0; i < m_n; ++i) {
            mo[i] = sqrt(mo[i] * mo[i] + io[i] * io[i]);
        }

        delete[] io;*/
    }

    void inverse(const double *ri, const double *ii, double *ro) {
        std::copy_n(ri, m_c, m_freal.get());
        std::copy_n(ii, m_c, m_fimag.get());
        fftwf_execute(m_plani);
        auto scale = 1.0f / m_n;
        std::transform(m_fbuf.get(),m_fbuf.get() + m_n, ro, [scale](auto x){return x * scale;});
        // kiss_fftr.h says
        // "input freqdata has nfft/2+1 complex points"

/*        for (int i = 0; i < m_n/2 + 1; ++i) {
            m_c[i].r = ri[i];
            m_c[i].i = ii[i];
        }

        kiss_fftri(m_plani, m_c, ro);

        double scale = 1.0 / m_n;

        for (int i = 0; i < m_n; ++i) {
            ro[i] *= scale;
        }*/
    }

private:
    int m_n;
    int m_c{m_n / 2 + 1};
    std::unique_ptr<float[],fftwf_deleter<float>> m_fbuf;
    std::unique_ptr<float[],fftwf_deleter<float>> m_freal;
    std::unique_ptr<float[],fftwf_deleter<float>> m_fimag;

    fftwf_plan   m_planf;
    fftwf_plan   m_plani;

/*    kiss_fftr_cfg m_planf;
    kiss_fftr_cfg m_plani;
    kiss_fft_cpx *m_c;*/
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



