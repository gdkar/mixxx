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

#ifndef FILTER_H
#define FILTER_H

#include "maths/MathUtilities.h"
#include <memory>
/**
 * Filter specification. For a filter of order ord, the ACoeffs and
 * BCoeffs arrays must point to ord+1 values each. ACoeffs provides
 * the denominator and BCoeffs the numerator coefficients of the
 * filter.
 */
struct FilterConfig{
    unsigned int ord;
    float* ACoeffs;
    float* BCoeffs;
};

/**
 * Digital filter specified through FilterConfig structure.
 */
template<class T>
class Filter
{
public:
    Filter() = default;
    Filter(Filter &&) noexcept = default;
    Filter&operator=(Filter && ) noexcept = default;

    Filter( FilterConfig c)
    : m_ord(c.ord)
    {
        std::copy_n(c.ACoeffs,m_ord + 1,&m_ACoeffs[0]);
        std::copy_n(c.BCoeffs,m_ord + 1,&m_BCoeffs[0]);
    }
    virtual ~Filter() = default;

    void reset()
    {
        std::fill_n(&m_inBuffer[0],m_ord+1, T{});
        std::fill_n(&m_outBuffer[0],m_ord+1, T{});
    }
    template<class I, class O>
    void process( I src, O dst, size_t length)
    {
        auto send = src + length;
        auto in_beg = m_inBuffer.get(); auto in_mid = in_beg + m_ord; auto in_end = in_mid + 1;
        auto out_beg= m_outBuffer.get();auto out_mid= out_beg + m_ord;auto out_end= out_mid + 1;
        auto b_beg = m_BCoeffs.get();//auto b_end = b_beg + m_ord + 1;
        auto a_beg = m_ACoeffs.get();auto a_mid = a_beg + 1;//auto a_end = a_mid + m_ord;
        for(; src != send;) {
            *in_mid = *src++;
            std::rotate(in_beg, in_mid, in_end);
            auto xout = std::inner_product(in_beg, in_end, b_beg, T{});
            xout -= std::inner_product(out_beg, out_mid, a_mid, T{});
            *out_mid = *dst++ = xout;
            std::rotate(out_beg, out_mid, out_end);
        }
    }

private:
//    void initialise( FilterConfig Config );
//    void deInitialise();

    size_t m_ord{};

    std::unique_ptr<T[]> m_inBuffer = std::make_unique<T[]>(m_ord+1);
    std::unique_ptr<T[]> m_outBuffer = std::make_unique<T[]>(m_ord+1);

    std::unique_ptr<T[]> m_ACoeffs = std::make_unique<T[]>(m_ord+1);
    std::unique_ptr<T[]> m_BCoeffs = std::make_unique<T[]>(m_ord+1);

};

#endif
