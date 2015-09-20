/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2014 Particular Programs Ltd.

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

_Pragma("once")
#include <sys/types.h>

//#define DEBUG_RINGBUFFER 1

#include <atomic>
#include <algorithm>
#include <utility>
#include <iostream>
#include <memory>
namespace{

    template<typename T>
    T _roundup(T x) {x--;x|=x>>1;x|=x>>2;x|=x>>4;x|=x>>8;x|=x>>16;return x++;}
    template<>
    __int128 _roundup(__int128 x) {x--;x|=x>>1;x|=x>>2;x|=x>>4;x|=x>>8;x|=x>>16;x|=x>>32;x|=x>>64;return x++;}
    template<>

    long _roundup(long x) {x--;x|=x>>1;x|=x>>2;x|=x>>4;x|=x>>8;x|=x>>16;x|=x>>32;return x++;}
    template<>
    int _roundup(int x) {x--;x|=x>>1;x|=x>>2;x|=x>>4;x|=x>>8;x|=x>>16;return x++;}
    template<>
    short _roundup(short x) {x--;x|=x>>1;x|=x>>2;x|=x>>4;x|=x>>8;return x++;}
    template<>
    char _roundup(char x) {x--;x|=x>>1;x|=x>>2;x|=x>>4;return x++;}
}
/**
 * RingBuffer implements a lock-free ring buffer for one writer and
 * one reader, that is to be used to store a sample type T.
 *
 * RingBuffer is thread-safe provided only one thread writes and only
 * one thread reads.
 */

template <typename T>
class RingBuffer
{
public:
    /**
     * Create a ring buffer with room to write n samples.
     *
     * Note that the internal storage size will actually be n+1
     * samples, as one element is unavailable for administrative
     * reasons.  Since the ring buffer performs best if its size is a
     * power of two, this means n should ideally be some power of two
     * minus one.
     */
    typedef off_t size_type;
    RingBuffer(size_type  n);
    virtual ~RingBuffer() = default;
    /**
     * Return the total capacity of the ring buffer in samples.
     * (This is the argument n passed to the constructor.)
     */
    virtual size_type size () const;
    /**
     * Return a new ring buffer (allocated with "new" -- caller must
     * delete when no longer needed) of the given size, containing the
     * same data as this one.  If another thread reads from or writes
     * to this buffer during the call, the results may be incomplete
     * or inconsistent.  If this buffer's data will not fit in the new
     * size, the contents are undefined.
     */
    virtual RingBuffer<T> *resized(size_type newSize) const;
    /**
     * Reset read and write pointers, thus emptying the buffer.
     * Should be called from the write thread.
     */
    virtual void reset();
    /**
     * Return the amount of data available for reading, in samples.
     */
    virtual size_type getReadSpace() const;
    /**
     * Return the amount of space available for writing, in samples.
     */
    virtual size_type getWriteSpace() const;
    /**
     * Read n samples from the buffer.  If fewer than n are available,
     * the remainder will be zeroed out.  Returns the number of
     * samples actually read.
     *
     * This is a template function, taking an argument S for the target
     * sample type, which is permitted to differ from T if the two
     * types are compatible for arithmetic operations.
     */
    virtual size_type read(T *const destination, size_type n);

    /**
     * Read one sample from the buffer.  If no sample is available,
     * this will silently return zero.  Calling this repeatedly is
     * obviously slower than calling read once, but it may be good
     * enough if you don't want to allocate a buffer to read into.
     */
    virtual T readOne();
    /**
     * Read n samples from the buffer, if available, without advancing
     * the read pointer -- i.e. a subsequent read() or skip() will be
     * necessary to empty the buffer.  If fewer than n are available,
     * the remainder will be zeroed out.  Returns the number of
     * samples actually read.
     */
    virtual size_type peek(T *const destination, size_type n) const;
    /**
     * Read one sample from the buffer, if available, without
     * advancing the read pointer -- i.e. a subsequent read() or
     * skip() will be necessary to empty the buffer.  Returns zero if
     * no sample was available.
     */
    virtual T peekOne() const;
    /**
     * Pretend to read n samples from the buffer, without actually
     * returning them (i.e. discard the next n samples).  Returns the
     * number of samples actually available for discarding.
     */
    virtual size_type skip(size_type n);
    /**
     * Write n samples to the buffer.  If insufficient space is
     * available, not all samples may actually be written.  Returns
     * the number of samples actually written.
     *
     * This is a template function, taking an argument S for the source
     * sample type, which is permitted to differ from T if the two
     * types are compatible for assignment.
     */
    virtual size_type reserveReadSpace(size_type, T*&,size_type&,T*&,size_type&);
    virtual size_type reserveWriteSpace(size_type,T*&,size_type&,T*&,size_type&);
    virtual size_type releaseReadSpace(size_type);
    virtual size_type releaseWriteSpace(size_type);
    virtual size_type write(const T *const  source, size_type n);
    /**
     * Write n zero-value samples to the buffer.  If insufficient
     * space is available, not all zeros may actually be written.
     * Returns the number of zeroes actually written.
     */
    virtual size_type zero(size_type n);
    RingBuffer(const RingBuffer &) = delete;
    RingBuffer(RingBuffer && ) = default;
    RingBuffer &operator=(const RingBuffer &) = delete;
    RingBuffer &operator=(RingBuffer &&) = default;
protected:
    const size_type m_size;
    const size_type m_mask;
    std::unique_ptr<T[]>       m_buffer { nullptr };
    std::atomic<size_type>    m_writer { 0 };
    std::atomic<size_type>    m_reader { 0 };
    size_type readSpaceFor(size_type w, size_type  r) const {
        return ( w > r ) ? w - r : 0;
    }
    size_type writeSpaceFor(size_type w, size_type r) const {
        return ( r + m_size > w ) ? r + m_size - w : 0;
    }
};
template <typename T>
RingBuffer<T>::RingBuffer(typename RingBuffer<T>::size_type n) :
    m_size(_roundup(n)),
    m_mask(_roundup(n)-1),
    m_buffer(std::make_unique<T[]>(m_size)){}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::size() const{return m_size;}
template <typename T>
RingBuffer<T> *
RingBuffer<T>::resized(typename RingBuffer<T>::size_type newSize) const{
    auto newBuffer = new RingBuffer<T>(newSize);
    auto w = m_writer.load();
    auto r = m_reader.load();
    while (r != w) {
        auto value = m_buffer[r&m_mask];
        newBuffer->write(&value, 1);
        r++;
    }
    return newBuffer;
}
template <typename T>
void
RingBuffer<T>::reset(){ m_reader.store(m_writer.load()); }
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::getReadSpace() const {return readSpaceFor(m_writer.load(), m_reader.load());}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::getWriteSpace() const {return writeSpaceFor(m_writer.load(), m_reader.load());}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::read(T *const destination, typename RingBuffer<T>::size_type n){
    auto w = m_writer.load();
    auto r = m_reader.load();
    if((n=std::min(n,readSpaceFor(w,r))))
    {
        auto off = r&m_mask;
        auto here = m_size - off;
        auto bufbase = &m_buffer[off];
        if (here >= n) {
            std::copy_n(bufbase, n, destination );
        } else {
            std::copy_n ( bufbase, here, destination );
            std::copy_n ( &m_buffer[0], n-here, &destination[here] );
        }
        m_reader += n;
    }
    return n;
}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::reserveWriteSpace(size_type n, T*&ptr0, size_type &size0,T*&ptr1,size_type &size1)
{
    auto w = m_writer.load();
    auto r = m_reader.load();
    size0 = size1= 0;;
    ptr0  = ptr1 = nullptr;
    if((n=std::min(n,writeSpaceFor(w,r))))
    {
        auto off = w&m_mask;
        size0    = std::min(m_size - off,n);
        ptr0     = &m_buffer[off];
        if ( size0 < n )
        {
            ptr1  = &m_buffer[0];
            size1 = n - size0;
        }
        else
        {
            ptr1 = nullptr;
            size1= 0;
        }
    }
    return n;
}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::reserveReadSpace(size_type n, T*&ptr0, size_type &size0,T*&ptr1,size_type &size1)
{
    auto w = m_writer.load();
    auto r = m_reader.load();
    size0 = size1= 0;;
    ptr0  = ptr1 = nullptr;
    if((n = std::min(n,readSpaceFor(w,r))))
    {
        auto off = r&m_mask;
        size0    = std::min(m_size - off,n);
        ptr0     = &m_buffer[off];
        if ( size0 < n )
        {
            ptr1  = &m_buffer[0];
            size1 = n - size0;
        }
        else
        {
            ptr1 = nullptr;
            size1= 0;
        }
    }
    return n;
}
template <typename T>
T RingBuffer<T>::readOne(){
    auto w = m_writer.load();
    auto r = m_reader.load();
    if (w == r) {return T{};}
    auto value = m_buffer[r&m_mask];
    m_reader += 1;
    return value;
}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::peek(T *const destination,typename RingBuffer<T>::size_type n) const{
    auto w = m_writer.load();
    auto r = m_reader.load();
    if((n = std::min(n,readSpaceFor(w,r))))
    {
        auto off = r &m_mask;
        auto here = m_size - off;
        auto bufbase = &m_buffer[off];
        if (here >= n) {
            std::copy_n ( bufbase, n, destination );
        } else {
            std::copy_n ( bufbase, here, destination );
            std::copy_n ( &m_buffer[0], n-here,&destination[here]);
        }
    }
    return n;
}
template <typename T>
T RingBuffer<T>::peekOne() const{
    auto w = m_writer.load();
    auto r = m_reader.load();
    if (w == r) { return T{}; }
    return m_buffer[r&m_mask];
}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::skip(typename RingBuffer<T>::size_type  n){
    auto w = m_writer.load();
    auto r = m_reader.load();
    n = std::min(n,readSpaceFor(w,r));;
    m_reader += n;
    return n;
}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::releaseReadSpace(typename RingBuffer<T>::size_type  n){
    auto w = m_writer.load();
    auto r = m_reader.load();
    n = std::min(n,readSpaceFor(w,r));
    m_reader += n;
    return n;
}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::releaseWriteSpace(typename RingBuffer<T>::size_type  n){
    auto w = m_writer.load();
    auto r = m_reader.load();
    n = std::min(n,writeSpaceFor(w,r));
    m_writer += n;
    return n;
}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::write(const T *const source, typename RingBuffer<T>::size_type n){
    auto w = m_writer.load();
    auto r = m_reader.load();
    if((n = std::min(n,writeSpaceFor(w,r))))
    {
        auto off = w & m_mask;
        auto here = m_size - off;
        auto bufbase = &m_buffer[off];
        if (here >= n) {
            std::copy_n ( source, n, bufbase );
        } else {
            std::copy_n ( source, here, bufbase );
            std::copy_n ( source + here, n - here, &m_buffer[0] );
        }
        m_writer += n;
    }
    return n;
}
template <typename T>
typename RingBuffer<T>::size_type
RingBuffer<T>::zero(typename RingBuffer<T>::size_type  n){
    auto w = m_writer.load();
    auto r = m_reader.load();
    if((n = std::min(n,writeSpaceFor(w,r))))
    {
        auto off = w &m_mask;
        auto here = m_size - off;
        auto bufbase = &m_buffer[off];
        if (here >= n) {std::fill_n(bufbase, n, T{});}
        else {
            std::fill_n ( bufbase, here, T{});
            std::fill_n ( &m_buffer[0], n - here, T{});
        }
        m_writer += n;
    }
    return n;
}
