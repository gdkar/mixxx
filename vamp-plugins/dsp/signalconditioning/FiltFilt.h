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

#ifndef FILTFILT_H
#define FILTFILT_H

#include "Filter.h"

/**
 * Zero-phase digital filter, implemented by processing the data
 * through a filter specified by the given FilterConfig structure (see
 * Filter) and then processing it again in reverse.
 */
template<class T>
class FiltFilt
{
public:
    FiltFilt( FilterConfig Config )
    : m_ord(Config.ord)
    , m_filter(Config)
    , m_filtScratchPre(3 * m_ord)
    , m_filtScratchPost(3 * m_ord)
    , m_filterConfig{Config}
    { }
    FiltFilt() = default;
    FiltFilt(FiltFilt &&) noexcept = default;
    FiltFilt&operator=(FiltFilt&&) noexcept = default;
    virtual ~FiltFilt() = default;

    void reset()
    {
        m_filter.reset();
    }
    template<class I, class O>
    void process( I src, O dst, size_t length )
    {
        auto send = src + length;
        auto dend = dst + length;
        auto rdend = std::make_reverse_iterator(dend);
        std::transform(src,src + m_filtScratchPre.size(),m_filtScratchPre.rend(),
            [offset=(*src * 2)](auto x){return offset - x;});
        std::transform(send - m_filtScratchPost.size(),send,
            m_filtScratchPost.rend(),
            [offset=(*(send-1)*2)](auto x){return offset - x;});
        m_filter.reset();
        m_filter.process(m_filtScratchPre.begin(), m_filtScratchPre.begin(), m_filtScratchPre.size());
        m_filter.process(src,dst,length);
        m_filter.process(m_filtScratchPost.begin(),m_filtScratchPost.begin(),m_filtScratchPost.size());
        m_filter.reset();
        m_filter.process(m_filtScratchPost.rbegin(),m_filtScratchPost.rbegin(),m_filtScratchPost.size());
        m_filter.process(rdend,rdend,length);
        m_filter.process(m_filtScratchPre.rbegin(),m_filtScratchPre.rbegin(),m_filtScratchPre.size());
    }

private:

    size_t m_ord;

    Filter<T>       m_filter;

    std::vector<T>  m_filtScratchPre;
    std::vector<T>  m_filtScratchPost;

    FilterConfig m_filterConfig;
};

#endif
