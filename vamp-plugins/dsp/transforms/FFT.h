/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP Library

    Centre for Digital Music, Queen Mary, University of London.
*/

#ifndef FFT_H
#define FFT_H
#include "maths/MathUtilities.h"
#include <alloca.h>
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
    void _process(bool inverse,
                 const float *realIn, const float *imagIn,
                 float*realOut, float *imagOut);

    template<class I, class O>
    void process(bool inverse,
        const I *realIn, const I *imagIn,
        O *realOut, O *imagOut)
    {
        using float_ptr = float*;
        auto rin = float_ptr{},iin=float_ptr{},rout=float_ptr{},iout=float_ptr{};
        if(realIn) {
            rin = (float*)alloca(m_size * sizeof(float));
            std::copy_n(realIn,m_size,rin);
        }
        if(imagIn) {
            iin = (float*)alloca(m_size * sizeof(float));
            std::copy_n(imagIn,m_size,iin);
        }
        if(realOut)
            rout = (float*)alloca(m_size * sizeof(float));
        if(imagOut)
            iout = (float*)alloca(m_size * sizeof(float));
        process(inverse, (const float*)rin,(const float*)iin, (float*)rout, (float*)iout);
        if(realOut)
            std::copy_n(rout,m_size,realOut);
        if(imagOut)
            std::copy_n(iout,m_size,imagOut);
    }
private:
    int m_size;
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
    void _forward(const float *realIn,
                 float *realOut, float *imagOut);

    template<class I, class O>
    void forward(const I *realIn, O *realOut, O *imagOut)
    {
        using float_ptr = float*;
        auto rin = float_ptr{},rout=float_ptr{},iout=float_ptr{};
        if(realIn) {
            rin = (float*)alloca(m_size * sizeof(float));
            std::copy_n(realIn,m_size,rin);
        }
        if(realOut)
            rout = (float*)alloca(m_size* sizeof(float));
        if(imagOut)
            iout = (float*)alloca(m_size* sizeof(float));
        forward((const float*)rin, (float*)rout, (float*)iout);
        if(realOut)
            std::copy_n(rout,m_size ,realOut);
        if(imagOut)
            std::copy_n(iout,m_size ,imagOut);
    }


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
    void _forwardMagnitude(const float *realIn, float *magOut);
    template<class I, class O>
    void forwardMagnitude(const I *realIn, O *realOut)
    {
        using float_ptr = float*;
        auto rin = float_ptr{},rout=float_ptr{};
        if(!realIn || !realOut)
            return;
        rin = (float*)alloca(m_size * sizeof(float));
        std::copy_n(realIn,m_size,rin);
        rout = (float*)alloca(m_size* sizeof(float));

        forwardMagnitude((const float*)rin, (float*)rout);
        std::copy_n(rout,m_size,realOut);
    }
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
    void _inverse(const float *realIn, const float *imagIn,
                 float *realOut);
    template<class I, class O>
    void inverse(const I *realIn, const I *imagIn,O *realOut)
    {
        using float_ptr = float*;
        auto rin = float_ptr{},iin = float_ptr{},rout=float_ptr{};
        if(realIn) {
            rin = (float*)alloca((m_size/2+1) * sizeof(float));
            std::copy_n(realIn,(m_size/2+1),rin);
        }
        if(imagIn) {
            iin = (float*)alloca((m_size/2+1)* sizeof(float));
            std::copy_n(imagIn,(m_size/2+1),iin);
        }
        if(realOut)
            rout = (float*)alloca(m_size * sizeof(float));
        inverse((const float*)rin, (const float*)iin, (float*)rout);
        if(realOut)
            std::copy_n(rout,m_size,realOut);
    }
private:
    int m_size;
    class D;
    D *m_d;
};

#endif
