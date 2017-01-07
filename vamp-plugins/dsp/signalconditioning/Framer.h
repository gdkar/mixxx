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

#ifndef FRAMER_H
#define FRAMER_H

//#include <io.h>
#include <cstdio>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <iterator>
#include <functional>
#include <type_traits>


template<class T, class I = const T*>
class Framer
{
public:
    using value_type = T;
    using iterator   = I;
    using difference_type = typename std::iterator_traits<I>::difference_type;
    using size_type = std::size_t;

    Framer() { }

    void setSource( iterator it, size_type length )
    {
        m_dataLength = length;
        m_baseLength = length;
        m_baseBuffer = it;
        m_srcBuffer  = it;
        m_maxFrames  = (length + m_stepSize - 1) / m_stepSize;
    }
    size_type getMaxNoFrames() const
    {
        return m_maxFrames;
    }
    template<class O>
    void getFrame( O dst)
    {
        auto have = std::min(m_dataLength, m_frameLength);
        auto skip = std::min(m_dataLength, m_stepSize);
        auto havenot = m_frameLength - have;
        if(have) {
            dst = std::copy(m_srcBuffer, m_srcBuffer + have, dst);
        }
        if(skip) {
            m_srcBuffer += skip;
            m_dataLength-= skip;
        }
        if(havenot) {
            std::fill_n(dst, havenot, T{});
        }
    }
    void configure( size_type frameLength, size_type hop )
    {
        m_frameLength = frameLength;
        m_stepSize    = hop;
        m_maxFrames   = m_framesRead + (m_dataLength + m_stepSize - 1)/m_stepSize;
    }
    virtual ~Framer() { }

    void resetCounters()
    {
        m_framesRead = 0;
        m_dataLength = m_baseLength;
        m_srcBuffer  = m_baseBuffer;
        m_maxFrames  = (m_dataLength + m_stepSize - 1)/m_stepSize;
    }

private:

    size_type m_dataLength;		// DataLength (samples)
    size_type m_framesRead;		// Read Frames Index

    iterator  m_baseBuffer;
    size_type m_baseLength;		// DataLength (samples)
    iterator  m_srcBuffer;
    size_type m_frameLength;		// Analysis Frame Length
    size_type m_stepSize;		// Analysis Frame Stride
    size_type m_maxFrames;
};

#endif
