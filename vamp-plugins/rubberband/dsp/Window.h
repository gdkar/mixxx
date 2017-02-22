/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2015 Particular Programs Ltd.

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

#ifndef _RUBBERBAND_WINDOW_H_
#define _RUBBERBAND_WINDOW_H_

#include <cmath>
#include <cstdlib>
#include <map>

#include "rubberband/system/sysutils.hpp"
#include "KaiserWindow.hpp"
namespace RubberBand {

enum WindowType {
    InvalidWindow,
    RectangularWindow,
    BartlettWindow,
    HammingWindow,
    HanningWindow,
    BlackmanWindow,
    GaussianWindow,
    ParzenWindow,
    NuttallWindow,
    BlackmanHarrisWindow,
    KaiserWindow,
    SincWindow,
};

template <typename T>
class Window
{
public:
    /**
     * Construct a windower of the given type.
     */
    using value_type = T;
    using storage_type = simd_vec<T>;
    using reference    = typename storage_type::reference;
    using const_reference    = typename storage_type::const_reference;
    using pointer = typename storage_type::pointer;
    using const_pointer = typename storage_type::const_pointer;
    using size_type = typename storage_type::size_type;
    using difference_type = typename storage_type::difference_type;
    Window() = default;
    Window(WindowType type, size_type size, T shape = T{3})
    : m_type(type), m_size(size), m_cache(0), m_shape(shape)
    {
        encache();
    }
    Window(Window &&w) noexcept = default;
    Window &operator=( Window &&w) noexcept = default;

    Window(const Window &w)  = default;
    Window &operator=(const Window &w) = default;
    virtual ~Window() = default;

    template<class I>
    void cut(I it) const {
        v_multiply(it , data(), m_size);
    }

    template<class O, class I>
    void cut(I src, O dst) const {
        v_multiply(dst, src, data(), m_size);
    }

    template<class O, class G>
    void add(O dst, G scale) const {
        v_add_with_gain(dst, data(), scale, m_size);
    }
    T shape() const { return m_shape;}
    const_reference operator[](difference_type x) const { return m_cache[x];}
    const_reference at(difference_type x) const { return m_cache.at(x);}
    const_pointer begin() const { return &m_cache[0];}
    const_pointer end  () const { return &m_cache[0] + m_size;}
    const_pointer cbegin() const { return &m_cache[0];}
    const_pointer cend  () const { return &m_cache[0] + m_size;}
    void resize(size_type _size )
    {
        if(_size != size()) {
            m_size = _size;
            m_cache.resize(_size);
            encache();
        }
    }
    void retype(WindowType _type)
    {
        if(_type != type()) {
            m_type = _type;
            encache();
        }
    }
    const_pointer data() const { return m_cache.data();}
    size_type size() const { return m_size;}
    WindowType type() const { return m_type;}
    value_type area() const { return m_area;}
    WindowType getType() const { return type(); }
    size_type getSize() const { return size(); }
    T getArea() const { return area(); }
    T getValue(size_type i) const { return m_cache[i]; }

protected:
    WindowType m_type{InvalidWindow};
    size_type m_size{};
    T m_shape{};
    storage_type m_cache{m_size, bs::allocator<T>{}};
    T m_area{};
    void encache();
};
template <typename T>
void Window<T>::encache()
{
    m_cache.resize(m_size);
    auto n = m_size;
    std::fill(m_cache.begin(),m_cache.end(),T{});
    auto cosinewin = [&] (auto & multi, T a0, T a1, T a2, T a3) {
        auto n = m_size;
        auto w1 = bs::Twopi<T>() / n;
        auto w2 = 2 * w1, w3 = 3 * w1;
        for (auto i = decltype(n){}; i < n; ++i) {
            multi[i] = ( a0 - a1 * bs::cos(i * w1)
                             + a2 * bs::cos(i * w2)
                             - a3 * bs::cos(i * w3));
        }
    };
    switch (m_type) {

    case RectangularWindow:
	for (auto i = 0; i < n; ++i) {
	    m_cache[i] = 0.5;
	}
	break;

    case BartlettWindow:
	for (auto i = 0; i < n/2; ++i) {
	    m_cache[i] = (i / T(n/2));
	    m_cache[i + n/2] = (1.0 - (i / T(n/2)));
	}
	break;

    case HammingWindow:
        cosinewin(m_cache, 0.54, 0.46, 0.0, 0.0);
	break;

    case HanningWindow:
        cosinewin(m_cache, 0.50, 0.50, 0.0, 0.0);
	break;

    case BlackmanWindow:
        cosinewin(m_cache, 0.42, 0.50, 0.08, 0.0);
	break;

    case GaussianWindow:
	for (auto i = 0; i < n; ++i) {
            m_cache[i] = bs::exp2( - bs::sqr((i - (n-1)*0.5) / ((n-1)*0.5 / 3)));
	}
	break;
    case SincWindow: {
        auto half = n/2;
        m_cache[half] = 1;
        for(auto i = 1; i + (n/2) < n; ++i) {
            auto a = bs::sinc(i * bs::Twopi<T>() / m_shape);
            m_cache[half+i] = a;
            m_cache[half-i] = a;
        }
        break;
    }
    case ParzenWindow:
    {
        auto  N = n-1;
        for (auto i = 0; i < N/4; ++i) {
            auto m = 2 * bs::pow(1.0 - (T(N)/2 - i) / (T(N)/2), 3);
            m_cache[i]   = m;
            m_cache[N-i] = m;
        }
        for (auto i = N/4; i <= N/2; ++i) {
            auto wn = i - N/2;
            auto m = 1.0 - 6 * bs::pow(wn / (T(N)/2), 2) * (1.0 - abs(wn) / (T(N)/2));
            m_cache[i]   = m;
            m_cache[N-i] = m;
        }
        break;
    }

    case NuttallWindow:
        cosinewin(m_cache, 0.3635819, 0.4891775, 0.1365995, 0.0106411);
	break;

    case BlackmanHarrisWindow:
        cosinewin(m_cache, 0.35875, 0.48829, 0.14128, 0.01168);
        break;
    case KaiserWindow:
        make_kaiser_window(m_cache.begin(),m_cache.end(), m_shape);
        break;
    default:
        throw std::runtime_error("Window type not selected.");
    }
    m_area = std::accumulate(m_cache.begin(),m_cache.end(),value_type{}) / m_size;
}



}

#endif
