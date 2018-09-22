_Pragma("once")

#include <memory>
#include <climits>
#include <cfloat>
#include <exception>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <cmath>
#include <cstring>
#include <atomic>
#include <utility>
#include <algorithm>
#include <limits>
#include <numeric>

#include "util/range.hpp"
#include "util/math.h"

template<class T>
struct rb_iterator {
    using iterator_category = std::random_access_iterator_tag;
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = T&;
    using pointer         = T*;

    pointer         m_ptr{nullptr};
    difference_type m_idx{0};
    size_type       m_mask{0};
    constexpr rb_iterator() = default;
    constexpr rb_iterator(pointer ptr, difference_type idx, size_type _mask)
    : m_ptr(ptr)
    , m_idx(idx)
    , m_mask(_mask){}
    constexpr rb_iterator(const rb_iterator &o)       = default;
    constexpr rb_iterator(rb_iterator &&o) noexcept   = default;
    rb_iterator &operator=(const rb_iterator &o)      = default;
    rb_iterator &operator=(rb_iterator &&)            = default;
    template<class Y>
    constexpr rb_iterator(const rb_iterator<Y> &o) noexcept
    : m_ptr(o.m_ptr) , m_idx(o.m_idx) , m_mask(o.m_mask) { }
    template<class Y>
    constexpr rb_iterator(rb_iterator<Y> &&o) noexcept
    : m_ptr(o.m_ptr) , m_idx(o.m_idx) , m_mask(o.m_mask) { }
    template<class Y>
    rb_iterator &operator =(const rb_iterator<Y> &o) noexcept { m_ptr = o.m_ptr;m_idx = o.m_idx;m_mask = o.m_mask; return *this; }
    template<class Y>
    rb_iterator &operator =(rb_iterator<Y> &&o) noexcept { m_ptr = o.m_ptr;m_idx = o.m_idx;m_mask = o.m_mask; return *this;}

    void swap(rb_iterator &o)  noexcept
    {
        using std::swap; swap(m_ptr,o.m_ptr); swap(m_idx,o.m_idx);
    }

    constexpr size_type mask() const   { return m_mask;}
    constexpr size_type size() const   { return mask() + 1;}
    constexpr size_type offset() const { return m_idx & mask();}
    constexpr pointer   data() const   { return m_ptr;}
    constexpr pointer   get()  const   { return data() + offset();}

    constexpr bool operator ==(const rb_iterator& o) const { return m_ptr == o.m_ptr && m_idx == o.m_idx;}
    constexpr bool operator !=(const rb_iterator& o) const { return !(*this == o);}
    constexpr bool operator  <(const rb_iterator& o) const { return m_ptr == o.m_ptr && (m_idx < o.m_idx);}
    constexpr bool operator  >(const rb_iterator& o) const { return m_ptr == o.m_ptr && (m_idx > o.m_idx);}
    constexpr bool operator <=(const rb_iterator& o) const { return m_ptr == o.m_ptr && (m_idx <= o.m_idx);}
    constexpr bool operator >=(const rb_iterator& o) const { return m_ptr == o.m_ptr && (m_idx >= o.m_idx);}

    constexpr rb_iterator operator +(difference_type diff) { return rb_iterator(m_ptr, m_idx+diff, m_mask);}
    constexpr rb_iterator operator -(difference_type diff) { return rb_iterator(m_ptr, m_idx+diff, m_mask);}
    constexpr difference_type operator - (const rb_iterator &o) { return (m_ptr == o.m_ptr) ? m_idx - o.m_idx : throw std::invalid_argument("rb_iterators not to same object.");}

    rb_iterator &operator ++(){m_idx++;return *this;}
    rb_iterator &operator ++(int){auto ret = *this;++*this;return ret;}
    rb_iterator &operator --(){m_idx--;return *this;}
    rb_iterator &operator --(int){auto ret = *this;--*this;return ret;}
    rb_iterator &operator +=(difference_type diff) { m_idx += diff; return *this;}
    rb_iterator &operator -=(difference_type diff) { m_idx -= diff; return *this;}

    constexpr pointer   operator ->() const { return m_ptr + offset();}
    constexpr reference operator *() const { return m_ptr[offset()];}
    constexpr reference operator[](difference_type diff) const { return m_ptr[(m_idx + diff) & mask()];}
};

template<class T>
class ring_buffer {
protected:
    size_t                 m_capacity{0};
    size_t                 m_mask    {0};
    std::atomic<int64_t>   m_ridx{0};
    std::atomic<int64_t>   m_widx{0};
    std::unique_ptr<T[]>   m_data{};
public:
    using value_type = T;
    using size_type       = size_t;
    using difference_type = int64_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using iterator        = rb_iterator<T>;
    using const_iterator  = rb_iterator<const T>;
    using range_type      = range<iterator>;
    constexpr ring_buffer() = default;
    explicit ring_buffer(size_type cap)
    : m_capacity(roundUpToPowerOf2(cap))
    , m_mask(m_capacity - 1)
    , m_data(std::make_unique<T[]>(m_capacity)){}
    ring_buffer(ring_buffer &&o) noexcept
        : m_capacity(std::exchange(o.m_capacity,0))
        ,m_mask(std::exchange(o.m_mask,0))
        ,m_ridx(o.m_ridx.exchange(0))
        ,m_widx(o.m_widx.exchange(0))
    { m_data.swap(o.m_data);}
    ring_buffer &operator =(ring_buffer &&o) noexcept
    {
        m_ridx.exchange(o.m_ridx.exchange(0));
        m_widx.exchange(o.m_widx.exchange(0));
        m_data.swap(o.m_data);
    }
   ~ring_buffer() = default;
    constexpr size_type capacity() const { return m_capacity; }
    constexpr size_type mask()     const { return m_mask;}
    difference_type read_index()  const { return m_ridx.load(std::memory_order_acquire);}
    difference_type write_index() const { return m_widx.load(std::memory_order_acquire);}
    size_type       read_offset()  const { return read_index() & mask();}
    size_type       write_offset() const { return write_index() & mask();}
    pointer data() const { return m_data.get();}
    size_type size() const { return write_index() - read_index();}
    size_type space()const { return (read_index() + capacity()) - write_index();}
    bool      full() const { return (read_index() + capacity()) == write_index();}
    bool      empty()const { return read_index() == write_index();}

    range_type read_range()
    {
        return make_range(
            iterator(data(),read_index(),mask())
          , iterator(data(),write_index(),mask()));
    }
    range_type read_range(size_type _size, difference_type _diff = 0)
    {
        _size = std::min<size_type>(_size,std::max<difference_type>(size() - _diff,0));
        return make_range(
            iterator(data(),read_index() + _size,mask())
          , iterator(data(),read_index() + _size + _diff,mask()));
    }
    range_type write_range()
    {
        return make_range(
            iterator(data(),mask(),write_index())
          , iterator(data(),mask(),read_index() + capacity()));
    }
    range_type write_range(size_type _size)
    {
        _size = std::min<size_type>(_size,space());
        return make_range(
            iterator(data(),write_index(),mask())
          , iterator(data(),write_index() + _size,mask()));
    }
    size_type write_advance(size_type count)
    {
        count = std::min(count,space());
        m_widx.fetch_add(count,std::memory_order_release);
        return count;
    }
    size_type read_advance(size_type count)
    {
        count = std::min(count,size());
        m_ridx.fetch_add(count,std::memory_order_release);
        return count;
    }
    template<class Iter>
    size_type peek_n(Iter start, size_type count, difference_type offset = 0)
    {
        auto r = read_range((count = std::min(count, size()-offset)),offset);
        auto stop = std::copy_n(r.cbegin(), r.size(), start);
        return std::distance(start,stop);
    }
    template<class Iter>
    Iter peek(Iter start, Iter stop, difference_type offset = 0)
    {
        auto r = read_range(std::distance(start,stop),offset);
        return std::copy(r.cbegin(), r.cend(), start);
    }
    template<class Iter>
    size_type read_n(Iter start, size_type count)
    {
        auto r = read_range(count = std::min(count, size()));
        std::copy_n(r.begin(), r.size(), start);
        read_advance(r.size());
        return r.size();
    }
    template<class Iter>
    Iter read(Iter start, Iter stop)
    {
        auto r = read_range(std::distance(start,stop));
        stop = std::copy(r.cbegin(), r.cend(), start);
        m_ridx.fetch_add(r.size(), std::memory_order_release);
        return stop;
    }
    template<class Iter>
    size_type write_n(Iter start, size_type count)
    {
        auto r = write_range(count = std::min(count,space()));
        std::copy_n(start,r.size(),r.begin());
        write_advance(r.size());
        return r.size();
    }
    template<class Iter>
    Iter write(Iter start, Iter stop)
    {
        auto r = write_range(std::distance(start,stop));
        std::copy_n(start,r.size(),r.begin());
        m_widx.fetch_add(r.size(),std::memory_order_release);
        return std::next(start,r.size());
    }
    iterator begin()              { return iterator(data(),read_index(),mask());}
    const_iterator begin()  const { return const_iterator(data(),read_index(),mask());}
    const_iterator cbegin() const { return begin();}

    iterator end()                { return iterator(data(),write_index(),mask());}
    const_iterator end()    const { return const_iterator(data(),write_index(),mask());}
    const_iterator cend()   const { return end();}

    reference front()             { return m_data[read_index()];}
    const_reference front() const { return m_data[read_index()];}

    reference back ()             { return m_data[write_index()];}
    const_reference back () const { return m_data[write_index()];}

    void push_back(const_reference item)
    {
        if(!full()) { back() = item; m_widx.fetch_add(1);}
        else { throw std::invalid_argument("push onto full fifo.");}
    }
    void push_back(T && item)
    {
        if(!full()) { back() = std::forward<T>(item); m_widx.fetch_add(1);}
        else { throw std::invalid_argument("push onto full fifo.");}
    }
    template<class... Args>
    void emplace_back(Args &&...args)
    {
        if(!full()) {
            back().~T();
            ::new (&back()) T (std::forward<Args>(args)...);
            m_widx.fetch_add(1);
        }else {
            throw std::invalid_argument("push onto full fifo.");
        }
    }
    void pop_front()
    {
        if(!empty()) {m_ridx.fetch_add(1);}
        else { throw std::invalid_argument("pop from empty fifo.");}
    }
    bool try_push_back(const_reference item)
    {
        if(full())
            return false;
        back() = item;
        m_widx.fetch_add(1);
        return true;
    }
    bool try_push_back(T && item)
    {
        if(full())
            return false;
        back() = std::forward<T>(item);
        m_widx.fetch_add(1);
        return true;
    }
    template<class... Args>
    bool try_emplace_back(Args &&...args)
    {
        if(!full()) {
            back().~T();
            ::new (&back()) T (std::forward<Args>(args)...);
            m_widx.fetch_add(1);
            return true;
        }else {
            return false;
        }
    }

    bool try_pop_front(reference item)
    {
        if(empty())
            return false;
        item = front();
        m_ridx.fetch_add(1);
        return true;
    }
};
