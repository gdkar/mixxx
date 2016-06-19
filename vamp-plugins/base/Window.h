/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    QM DSP library
    Centre for Digital Music, Queen Mary, University of London.
    This file Copyright 2006 Chris Cannam.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <cmath>
#include <iostream>
#include <map>
#include <vector>

enum WindowType {
    FirstWindow,
    HammingWindow = FirstWindow,
    HanningWindow,
    BlackmanHarrisWindow,
    LastWindow = BlackmanHarrisWindow,
};

/**
 * Various shaped windows for sample frame conditioning, including
 * cosine windows (Hann etc) and triangular and rectangular windows.
 */
template <typename T>
class Window
{
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    using reference = typename std::vector<T>::reference;
    using const_reference = typename std::vector<T>::const_reference;
    using difference_type = typename std::vector<T>::difference_type;
    using size_type =typename std::vector<T>::size_type;
    /**
     * Construct a windower of the given type and size. 
     *
     * Note that the cosine windows are periodic by design, rather
     * than symmetrical. (A window of size N is equivalent to a
     * symmetrical window of size N+1 with the final element missing.)
     */
    Window(WindowType type, size_type size) : m_type(type), m_cache(size) { encache(); }
    Window(const Window &w) : m_type(w.m_type),m_cache(w.m_cache) {}
    template<typename U>
    Window(const Window<U> &w) : m_type(w.m_type),m_cache(w.getSize()) {encache();}
    Window(Window &&w) noexcept
    {
        swap(w);
    }
    Window &operator=(Window<T> &&w) noexcept
    {
        swap(w);
    }
    template<typename U>
    Window &operator=(const Window<U> &w)
    {
        m_type = w.m_type;
        m_cache.resize(w.getSize());
        encache();
        return *this;
    }
    Window &operator=(const Window<T> &w)
    {
	if (&w == this)
            return *this;
	m_type = w.m_type;
        m_cache= w.m_cache;
	return *this;
    }
    void swap(Window<T> &w) noexcept
    {
        std::swap(m_cache,w.m_cache);
        std::swap(m_type, w.m_type );
    }
    virtual ~Window() = default;
    template<typename Iter>
    void cut(Iter src) const
    {
        cut(src, src);
    }
    template<typename InputIt, typename OutputIt>
    void cut(InputIt src, OutputIt dst) const
    {
        std::transform(m_cache.cbegin(),m_cache.cend(),src,dst,
                [](auto x, auto y){return x*y;}
            );
    }
    WindowType getType() const { return m_type; }
    size_type getSize() const { return m_cache.size(); }
    std::vector<T> getWindowData() const { return m_cache; }
    template<typename OutputIt>
    void getWindowData(OutputIt it) const
    {
        std::copy(m_cache.cbegin(),m_cache.cend(),it);
    }
    const T &operator[](difference_type x){return m_cache[x];}
    T at(difference_type x) const { return m_cache.at(x);}
protected:
    WindowType m_type;
    std::vector<T> m_cache;
    void encache();
};
template <typename T>
void Window<T>::encache()
{
    auto f = static_cast<T>(2 * M_PI / m_cache.size());
    switch(m_type) {
        case HammingWindow:
            std::generate(m_cache.begin(),m_cache.end(),
                [f = f, i = 0]() mutable
                {
                    return T(0.54) - T(0.46) * std::cos((i++) * f);
                }
            );
        break;
        case HanningWindow:
            std::generate(m_cache.begin(),m_cache.end(),
                [f = f, i = 0]() mutable
                {
                    return T(0.5) - T(0.5) * std::cos((i++) * f);
                }
            );
        break;
        case BlackmanHarrisWindow:
            std::generate(m_cache.begin(),m_cache.end(),
                [f = f, i = 0]() mutable
                {
                    return T(0.35874) - T(0.48829 * std::cos( 1 * i * f))
                                      + T(0.14128 * std::cos( 2 * i * f))
                                      - T(0.01168 * std::cos( 3 * i * f));
                }
            );
        break;
    };
}

#endif
