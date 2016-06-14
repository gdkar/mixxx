/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
*/

#include "FFT.h"

#include "maths/MathUtilities.h"

#include "ext/kissfft/kiss_fft.h"
#include "ext/kissfft/kiss_fftr.h"

#include <cmath>

#include <iostream>

#include <stdexcept>

class FFT::D
{
    int m_n;
    kiss_fft_cfg m_planf;
    kiss_fft_cfg m_plani;
    std::unique_ptr<kiss_fft_cpx[]> m_kin;
    std::unique_ptr<kiss_fft_cpx[]> m_kout;
public:
    D(int n)
        : m_n(n)
        , m_kin(std::make_unique<kiss_fft_cpx[]>(n))
        , m_kout(std::make_unique<kiss_fft_cpx[]>(n))
        {
        m_planf = kiss_fft_alloc(m_n, 0, NULL, NULL);
        m_plani = kiss_fft_alloc(m_n, 1, NULL, NULL);
    }
   ~D() {
        kiss_fft_free(m_planf);
        kiss_fft_free(m_plani);
    }
    template<typename T>
    void process(bool inverse,
                 const T *ri,
                 const T *ii,
                 T *ro,
                 T *io)
    {
        if(ii) {
            std::transform(ri,ri+m_n,ii,m_kin.get(),[](auto r,auto i){return kiss_fft_cpx{kiss_fft_scalar(r),kiss_fft_scalar(i)};});
        }else{
            std::transform(ri,ri+m_n,m_kin.get(),[](auto r){return kiss_fft_cpx{kiss_fft_scalar(r),kiss_fft_scalar(0)};});
        }
        if (!inverse) {
            kiss_fft(m_planf, m_kin.get(), m_kout.get());
            std::transform(m_kout.get(),m_kout.get() + m_n, ro, [](auto x){return x.r;});
            std::transform(m_kout.get(),m_kout.get() + m_n, io, [](auto x){return x.i;});
        } else {
            kiss_fft(m_plani, m_kin.get(), m_kout.get());
            auto scale = 1.0f / m_n;
            std::transform(m_kout.get(),m_kout.get() + m_n, ro, [=](auto x){return x.r * scale;});
            std::transform(m_kout.get(),m_kout.get() + m_n, io, [=](auto x){return x.i * scale;});
        }
    }
};        

FFT::FFT(int n) : m_d(new D(n)) { }
FFT::~FFT() { delete m_d; }
void
FFT::process(bool inverse,
             const double *p_lpRealIn, const double *p_lpImagIn,
             double *p_lpRealOut, double *p_lpImagOut)
{
    m_d->process(inverse, p_lpRealIn, p_lpImagIn, p_lpRealOut, p_lpImagOut);
}
class FFTReal::D
{
public:
    D(int n) 
        : m_n(n)
        , m_c(std::make_unique<kiss_fft_cpx[]>(n))
        , m_r(std::make_unique<kiss_fft_scalar[]>(n))
    {
        if (n % 2) {
            throw std::invalid_argument
                ("nsamples must be even in FFTReal constructor");
        }
        m_planf = kiss_fftr_alloc(m_n, 0, NULL, NULL);
        m_plani = kiss_fftr_alloc(m_n, 1, NULL, NULL);
    }
    ~D() {
        kiss_fftr_free(m_planf);
        kiss_fftr_free(m_plani);
    }
    template<typename T>
    void forward(const T *ri, T *ro, T *io)
    {
        std::copy(ri,ri + m_n, m_r.get());
        kiss_fftr(m_planf, m_r.get(), m_c.get());
        for (int i = 0; i <= m_n/2; ++i) {
            ro[i] = m_c[i].r;
            io[i] = m_c[i].i;
        }
        for (int i = 0; i + 1 < m_n/2; ++i) {
            ro[m_n - i - 1] =  ro[i + 1];
            io[m_n - i - 1] = -io[i + 1];
        }
    }
    template<typename T>
    void forwardMagnitude(const T *ri, T *mo)
    {
        std::copy(ri,ri+m_n, m_r.get());
        kiss_fftr(m_planf, m_r.get(), m_c.get());
        std::transform(m_c.get(),m_c.get() + (m_n/2+1), mo, [](const auto &x){return std::hypot(x.r,x.i);});
        std::reverse_copy(mo,mo + m_n/2, mo + (m_n/2 + 1));
    }
    template<typename T>
    void inverse(const T *ri, const T *ii, T *ro)
    {
        // kiss_fftr.h says
        // "input freqdata has nfft/2+1 complex points"
        std::transform(ri,ri+(m_n/2+1),ii,&m_c[0],[](auto r, auto i){return kiss_fft_cpx{kiss_fft_scalar(r),kiss_fft_scalar(i)};});
        kiss_fftri(m_plani, m_c.get(), m_r.get());
        auto scale = 1.0f / m_n;
        std::transform(&m_r[0],&m_r[m_n],ro,[=](auto x){return x * scale;});
    }
private:
    int m_n;
    kiss_fftr_cfg m_planf;
    kiss_fftr_cfg m_plani;
    std::unique_ptr<kiss_fft_cpx[]>  m_c;
    std::unique_ptr<kiss_fft_scalar[]> m_r;
};
FFTReal::FFTReal(int n) : m_d(new D(n))  { }
FFTReal::~FFTReal() { delete m_d; }
void
FFTReal::forward(const double *ri, double *ro, double *io) { m_d->forward(ri, ro, io); }
void
FFTReal::forwardMagnitude(const double *ri, double*mo) { m_d->forwardMagnitude(ri, mo); }
void
FFTReal::inverse(const double *ri, const double *ii, double *ro) { m_d->inverse(ri, ii, ro); }
