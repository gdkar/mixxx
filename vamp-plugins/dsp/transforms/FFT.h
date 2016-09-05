/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
*/

#ifndef FFT_H
#define FFT_H
#include "maths/MathUtilities.h"

#include "ext/kissfft/kiss_fft.h"
#include "ext/kissfft/kiss_fftr.h"

#include <cmath>

#include <iostream>
#include <memory>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <cmath>
#include <utility>
#include <functional>
#include <type_traits>
#include <stdexcept>

class FFT
{
public:
    /**
     * Construct an FFT object to carry out complex-to-complex
     * transforms of size nsamples. nsamples does not have to be a
     * power of two.
     */
    FFT(int nsamples);
    ~FFT();

    /**
     * Carry out a forward or inverse transform (depending on the
     * value of inverse) of size nsamples, where nsamples is the value
     * provided to the constructor above.
     *
     * realIn and (where present) imagIn should contain nsamples each,
     * and realOut and imagOut should point to enough space to receive
     * nsamples each.
     *
     * imagIn may be NULL if the signal is real, but the other
     * pointers must be valid.
     *
     * The inverse transform is scaled by 1/nsamples.
     */
    template<class T>
    void process(bool inverse,
                 const T*realIn, const T*imagIn,
                 T*realOut, T*imagOut);

private:
    class D;
    D *m_d;
};
class FFTReal
{
public:
    /**
     * Construct an FFT object to carry out real-to-complex transforms
     * of size nsamples. nsamples does not have to be a power of two,
     * but it does have to be even. (Use the complex-complex FFT above
     * if you need an odd FFT size. This constructor will throw
     * std::invalid_argument if nsamples is odd.)
     */
    FFTReal(int nsamples);
    ~FFTReal();

    /**
     * Carry out a forward real-to-complex transform of size nsamples,
     * where nsamples is the value provided to the constructor above.
     *
     * realIn, realOut, and imagOut must point to (enough space for)
     * nsamples values. For consistency with the FFT class above, and
     * compatibility with existing code, the conjugate half of the
     * output is returned even though it is redundant.
     */
template<class T>
    void forward(const T*realIn,
                 T*realOut, T*imagOut);

    /**
     * Carry out a forward real-to-complex transform of size nsamples,
     * where nsamples is the value provided to the constructor
     * above. Return only the magnitudes of the complex output values.
     *
     * realIn and magOut must point to (enough space for) nsamples
     * values. For consistency with the FFT class above, and
     * compatibility with existing code, the conjugate half of the
     * output is returned even though it is redundant.
     */
template<class T>
    void forwardMagnitude(const T*realIn, T*magOut);

    /**
     * Carry out an inverse real transform (i.e. complex-to-real) of
     * size nsamples, where nsamples is the value provided to the
     * constructor above.
     *
     * realIn and imagIn should point to at least nsamples/2+1 values;
     * if more are provided, only the first nsamples/2+1 values of
     * each will be used (the conjugate half will always be deduced
     * from the first nsamples/2+1 rather than being read from the
     * input data).  realOut should point to enough space to receive
     * nsamples values.
     *
     * The inverse transform is scaled by 1/nsamples.
     */
template<class T>
    void inverse(const T*realIn, const T*imagIn,
                 T*realOut);

private:
    class D;
    D *m_d;
};
class FFT::D
{
public:
    D(int n) : m_n(n) {
        m_planf = kiss_fft_alloc(m_n, 0, NULL, NULL);
        m_plani = kiss_fft_alloc(m_n, 1, NULL, NULL);
        m_kin = new kiss_fft_cpx[m_n];
        m_kout = new kiss_fft_cpx[m_n];
    }

    ~D() {
        kiss_fft_free(m_planf);
        kiss_fft_free(m_plani);
        delete[] m_kin;
        delete[] m_kout;
    }

    template<class T>
    void process(bool inverse,
                 const T *ri,
                 const T *ii,
                 T *ro,
                 T *io) {

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

            auto  scale = T(1.0) / m_n;

            for (int i = 0; i < m_n; ++i) {
                ro[i] = m_kout[i].r * scale;
                io[i] = m_kout[i].i * scale;
            }
        }
    }
private:
    int m_n;
    kiss_fft_cfg m_planf;
    kiss_fft_cfg m_plani;
    kiss_fft_cpx *m_kin;
    kiss_fft_cpx *m_kout;
};
template<class T>
void
FFT::process(bool inverse,
             const T *p_lpRealIn, const T*p_lpImagIn,
             T *p_lpRealOut, T *p_lpImagOut)
{
    m_d->process(inverse,
                 p_lpRealIn, p_lpImagIn,
                 p_lpRealOut, p_lpImagOut);
}
class FFTReal::D
{
public:
    D(int n) : m_n(n) {
        if (n % 2) {
            throw std::invalid_argument
                ("nsamples must be even in FFTReal constructor");
        }
        m_planf = kiss_fftr_alloc(m_n, 0, NULL, NULL);
        m_plani = kiss_fftr_alloc(m_n, 1, NULL, NULL);
        m_c = new kiss_fft_cpx[m_n];
        m_r = new kiss_fft_scalar[m_n];
    }

    ~D() {
        kiss_fftr_free(m_planf);
        kiss_fftr_free(m_plani);
        delete[] m_c;
    }
    template<class T>
    void forward(const T *ri, T *ro, T *io) {
        std::copy_n(ri, m_n, m_r);
        kiss_fftr(m_planf, m_r, m_c);

        for (int i = 0; i <= m_n/2; ++i) {
            ro[i] = m_c[i].r;
            io[i] = m_c[i].i;
        }

        for (int i = 0; i + 1 < m_n/2; ++i) {
            ro[m_n - i - 1] =  ro[i + 1];
            io[m_n - i - 1] = -io[i + 1];
        }
    }
    template<class T>
    void forwardMagnitude(const T *ri, T *mo) {
        auto io = std::make_unique<T[]>(m_n);
        forward(ri, mo, io.get());
        for (int i = 0; i < m_n; ++i) {
            mo[i] = std::hypot(mo[i],io[i]);
        }
    }
    template<class T>
    void inverse(const T *ri, const T *ii, T *ro) {

        // kiss_fftr.h says
        // "input freqdata has nfft/2+1 complex points"

        for (int i = 0; i < m_n/2 + 1; ++i) {
            m_c[i].r = ri[i];
            m_c[i].i = ii[i];
        }
        kiss_fftri(m_plani, m_c, ro);

        auto scale = T(1) / m_n;

        for (int i = 0; i < m_n; ++i) {
            ro[i] *= scale;
        }
    }

private:
    int m_n;
    kiss_fftr_cfg m_planf;
    kiss_fftr_cfg m_plani;
    kiss_fft_cpx *m_c;
    kiss_fft_scalar *m_r;
};
template<class T>
void
FFTReal::forward(const T *ri, T *ro, T *io)
{
    m_d->forward(ri, ro, io);
}
template<class T>
void
FFTReal::forwardMagnitude(const T *ri, T *mo)
{
    m_d->forwardMagnitude(ri, mo);
}
template<class T>
void
FFTReal::inverse(const T*ri, const T*ii, T*ro)
{
    m_d->inverse(ri, ii, ro);
}
#endif
