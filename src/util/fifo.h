#ifndef FIFO_H
#define FIFO_H

#include <QtDebug>
#include <QMutex>
#include <memory>
#include <utility>
#include <functional>
#include <tuple>
#include <atomic>

//#include "pa_ringbuffer.h"

#include "util/math.h"

namespace detail {

template <class T, bool trivial>
class fifo;

template<class T>
class fifo<T,true> {
  public:
    using value_type      = T;

    using size_type       = std::size_t;
    using difference_type = std::int64_t;

    using reference       = T&;
    using pointer         = T*;

    using const_reference = const T&;
    using const_pointer   = const T*;


  protected:
    size_type        bufferSize{};
    difference_type  bigMask{};
    difference_type  smallMask{};
    std::unique_ptr<T[]> m_data{};
    std::atomic<difference_type> m_ridx{0};
    std::atomic<difference_type> m_widx{0};

  public:
    explicit fifo(size_type size)
    : bufferSize(roundUpToPowerOf2(size))
    , bigMask   ((bufferSize * 2) - 1)
    , smallMask ( bufferSize - 1)
    , m_data    ( std::make_unique<T[]>(bufferSize))
    { }
    virtual ~fifo() = default;
    bool empty() const
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        auto widx = m_widx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return ridx == widx;
    }
    bool full() const
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        auto widx = m_widx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return size_type((widx - ridx) & bigMask) == bufferSize;
    }
    reference front()
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return m_data[ridx & smallMask];
    }
    reference operator[](difference_type x)
    {
        auto idx = ((x>=0)?m_ridx:m_widx).load(std::memory_order_relaxed);
        auto off  = (idx + x) & smallMask;
        std::atomic_thread_fence(std::memory_order_acquire);
        return m_data[off];
    }
    const_reference operator[](difference_type x) const
    {
        auto idx = ((x>=0)?m_ridx:m_widx).load(std::memory_order_relaxed);
        auto off  = (idx + x) & smallMask;
        std::atomic_thread_fence(std::memory_order_acquire);
        return m_data[off];
    }
    const_reference front() const
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return m_data[ridx & smallMask];
    }
    reference back()
    {
        auto widx = m_widx.load(std::memory_order_relaxed);
        return m_data[widx & smallMask];
    }
    void pop_front()
    {
        m_ridx.fetch_add(1,std::memory_order_release);
    }
    void push_back(const_reference item)
    {
        back() = item;
        std::atomic_thread_fence(std::memory_order_release);
        m_widx.fetch_add(1,std::memory_order_relaxed);
    }
    void push_back(T &&item)
    {
        back() = std::forward<T>(item);
        std::atomic_thread_fence(std::memory_order_release);
        m_widx.fetch_add(1,std::memory_order_relaxed);
    }
    template<class... Args>
    void emplace_back(Args && ...args)
    {
        back().~T();
        ::new( static_cast<void*>(&back()) ) T (std::forward<Args>(args)...);
        std::atomic_thread_fence(std::memory_order_release);
        m_widx.fetch_add(1,std::memory_order_relaxed);
    }
    size_type capacity() const
    {
        return bufferSize;
    }
    size_type readAvailable() const
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        auto widx = m_widx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return (widx - ridx) & bigMask;
    }
    size_type writeAvailable() const
    {
        return bufferSize - readAvailable();
    }
    template<class It>
    size_type read(It pData, size_type count)
    {
        auto data0 = pointer{},data1=pointer{};
        auto size0 = size_type{}, size1=size_type{};
        count = acquireReadRegions(count, &data0, &size0, &data1, &size1);
        if(size0) {
            pData = std::copy_n(data0, size0, pData);
            if(size1) {
                std::copy_n(data1, size1, pData);
            }
        }
        return releaseReadRegions(count);
    }
    template<class It>
    size_type write(It pData, size_type count)
    {
        auto data0 = pointer{},data1=pointer{};
        auto size0 = size_type{}, size1=size_type{};
        count = acquireWriteRegions(count, &data0, &size0, &data1, &size1);
        if(size0) {
            std::copy_n(pData, size0, data0);
            std::advance(pData,size0);
            if(size1) {
                std::copy_n(pData, size1, data1);
            }
        }
        return releaseWriteRegions(count);
    }
    template<class It>
    void writeBlocking(It pData, size_type count) {
        auto written = size_type{0};
        while (written != count) {
            auto i = write(pData, count);
            pData   += i;
            written += i;
        }
    }
    size_type acquireWriteRegions(size_type count,
            pointer* dataPtr1, size_type *sizePtr1,
            pointer* dataPtr2, size_type* sizePtr2)
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        auto widx = m_widx.load(std::memory_order_relaxed);
        if((count = std::min<size_type>(count, bufferSize - ((widx-ridx)&bigMask)))) {
            auto woff = widx & smallMask;
            auto size1 = std::min<size_type>(count, bufferSize - woff);
           *sizePtr1 = size1;
           *dataPtr1 = &m_data[woff];
           *sizePtr2 = count - size1;
           *dataPtr2 = &m_data[0];
            std::atomic_thread_fence(std::memory_order_acquire);
        }else{
            *dataPtr1 = *dataPtr2 = nullptr;
            *sizePtr1 = *sizePtr2 = 0;
        }
        return count;
    }
    size_type releaseWriteRegions(size_type count)
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        auto widx = m_widx.load(std::memory_order_relaxed);
        count = std::min<size_type>(count, bufferSize - ((widx-ridx)&bigMask));
        std::atomic_thread_fence(std::memory_order_release);
        m_widx.fetch_add(count,std::memory_order_relaxed);
        return count;
    }
    size_type acquireReadRegions(size_type count,
            pointer* dataPtr1, size_type* sizePtr1,
            pointer* dataPtr2, size_type* sizePtr2)
    {
        auto ridx = m_ridx.load(std::memory_order_acquire);
        auto widx = m_widx.load(std::memory_order_acquire);
        if((count = std::min<size_type>(count, (widx-ridx)&bigMask))){
            auto roff = ridx & smallMask;
            auto size1 = std::min<size_type>(count, bufferSize - roff);
           *sizePtr1 = size1;
           *dataPtr1 = &m_data[roff];
           *sizePtr2 = count - size1;
           *dataPtr2 = &m_data[0];
            std::atomic_thread_fence(std::memory_order_acquire);
        }else{
            *dataPtr1 = *dataPtr2 = nullptr;
            *sizePtr1 = *sizePtr2 = 0;
        }
        return count;
    }
    size_type releaseReadRegions(size_type count)
    {
        auto ridx = m_ridx.load(std::memory_order_acquire);
        auto widx = m_widx.load(std::memory_order_acquire);
        count = std::min<size_type>(count, (widx-ridx)&bigMask);
        if(count){
            std::atomic_thread_fence(std::memory_order_release);
            m_ridx.fetch_add(count,std::memory_order_relaxed);
        }
        return count;
    }
    size_type flushReadData(size_type count)
    {
        return releaseReadRegions(count);
    }
    void reset(difference_type diff = 0)
    {
        m_ridx.store(diff); m_widx.store(diff);
    }
    difference_type read_index() const
    {
        return m_ridx.load(std::memory_order_acquire);
    }
    difference_type write_index() const
    {
        return m_widx.load(std::memory_order_acquire);
    }
};
template<class T>
class fifo<T,false> {
    using storage_type = std::aligned_storage_t<
        sizeof(T),
        std::alignment_of<T>::value
        >;
    static constexpr T& t_cast(storage_type &x) { return *reinterpret_cast<T*>(&x);}
    static constexpr const T& t_cast(const storage_type &x) { return *reinterpret_cast<const T*>(&x);}
  public:
    using value_type      = T;

    using size_type       = std::size_t;
    using difference_type = std::int64_t;

    using const_reference = const value_type &;
    using const_pointer   = const value_type *;

    using reference       = value_type &;
    using pointer         = value_type *;

  protected:
    size_type        bufferSize{};
    difference_type  bigMask{};
    difference_type  smallMask{};
    std::unique_ptr<storage_type[]> m_data{};
    std::atomic<difference_type>    m_ridx{0};
    std::atomic<difference_type>    m_widx{0};

  public:
    explicit fifo(size_type size)
    : bufferSize(roundUpToPowerOf2(size))
    , bigMask   ((bufferSize * 2) - 1)
    , smallMask ( bufferSize - 1)
    , m_data    ( std::make_unique<storage_type[]>(bufferSize))
    { }
   ~fifo()
    {
        clear();
    }
    bool empty() const
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        auto widx = m_widx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return ridx == widx;
    }
    bool full() const
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        auto widx = m_widx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return size_type((widx - ridx) & bigMask) == bufferSize;
    }
    reference front()
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return t_cast(m_data[ridx & smallMask]);
    }
    const_reference front() const
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return t_cast(m_data[ridx & smallMask]);
    }
    reference operator[](difference_type x)
    {
        auto idx = ((x>=0)?m_ridx:m_widx).load(std::memory_order_relaxed);
        auto off  = (idx + x) & smallMask;
        std::atomic_thread_fence(std::memory_order_acquire);
        return t_cast(m_data[off]);
    }
    const_reference operator[](difference_type x) const
    {
        auto idx = ((x>=0)?m_ridx:m_widx).load(std::memory_order_relaxed);
        auto off  = (idx + x) & smallMask;
        std::atomic_thread_fence(std::memory_order_acquire);
        return t_cast(m_data[off]);
    }
    reference back()
    {
        auto widx = m_widx.load(std::memory_order_relaxed);
        return t_cast(m_data[widx & smallMask]);
    }
    void pop_front()
    {
        front().~T();
        m_ridx.fetch_add(1,std::memory_order_release);
    }
    void push_back(const_reference item)
    {
        ::new(static_cast<void*>(&back())) T (item);
//        back() = item;
        std::atomic_thread_fence(std::memory_order_release);
        m_widx.fetch_add(1,std::memory_order_relaxed);
    }
    void push_back(T &&item)
    {
        ::new(static_cast<void*>(&back())) T (std::forward<T>(item));
        std::atomic_thread_fence(std::memory_order_release);
        m_widx.fetch_add(1,std::memory_order_relaxed);
    }
    template<class... Args>
    void emplace_back(Args && ...args)
    {
        ::new( static_cast<void*>(&back()) ) T (std::forward<Args>(args)...);
        std::atomic_thread_fence(std::memory_order_release);
        m_widx.fetch_add(1,std::memory_order_relaxed);
    }
    size_type capacity() const
    {
        return bufferSize;
    }
    size_type readAvailable() const
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        auto widx = m_widx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        return (widx - ridx) & bigMask;
    }
    size_type writeAvailable() const
    {
        return bufferSize - readAvailable();
    }
    template<class It>
    size_type read(It pData, size_type count)
    {
        auto data0 = pointer{},data1=pointer{};
        auto size0 = size_type{}, size1=size_type{};
        count = acquireReadRegions(count, &data0, &size0, &data1, &size1);
        if(size0) {
            pData = std::move(data0, data0 + size0, pData);
            for(auto ptr = data0; ptr < data0 + size0; ++ptr)
                ptr->~T();
            if(size1) {
                std::move(data1, data1 + size1, pData);
                for(auto ptr = data1; ptr < data1 + size1; ++ptr)
                    ptr->~T();
            }
        }
        return releaseReadRegions(count);
    }
    template<class It>
    size_type write(It pData, size_type count)
    {
        auto data0 = pointer{},data1=pointer{};
        auto size0 = size_type{}, size1=size_type{};
        count = acquireWriteRegions(count, &data0, &size0, &data1, &size1);
        if(size0) {
            for(auto ptr = data0; ptr != (data0+size0); (++ptr),(++pData))
                ::new(static_cast<void*>(ptr)) T (*pData);
            if(size1) {
                for(auto ptr = data1; ptr != (data1+size1); ++ptr,++pData)
                    ::new(static_cast<void*>(ptr)) T (*pData);
            }
        }
        return releaseWriteRegions(count);
    }
    template<class It>
    void writeBlocking(It pData, size_type count) {
        auto written = size_type{0};
        while (written != count) {
            auto i = write(pData, count);
            pData   += i;
            written += i;
        }
    }
    size_type acquireWriteRegions(size_type count,
            pointer* dataPtr1, size_type* sizePtr1,
            pointer* dataPtr2, size_type* sizePtr2)
    {
        auto ridx = m_ridx.load(std::memory_order_acquire);
        auto widx = m_widx.load(std::memory_order_acquire);
        if((count = std::min<size_type>(count, bufferSize - ((widx-ridx)&bigMask)))) {
            auto woff = widx & smallMask;
            auto size1 = std::min<size_type>(count, bufferSize - woff);
           *sizePtr1 = size1;
           *dataPtr1 = reinterpret_cast<T*>(&m_data[woff]);
           *sizePtr2 = count - size1;
           *dataPtr2 = reinterpret_cast<T*>(&m_data[0]);
            std::atomic_thread_fence(std::memory_order_acquire);
        }else{
            *dataPtr1 = *dataPtr2 = nullptr;
            *sizePtr1 = *sizePtr2 = 0;
        }
        return count;
    }
    size_type releaseWriteRegions(size_type count)
    {
        auto ridx = m_ridx.load(std::memory_order_acquire);
        auto widx = m_widx.load(std::memory_order_acquire);
        count = std::min<size_type>(count, bufferSize - ((widx-ridx)&bigMask));
        std::atomic_thread_fence(std::memory_order_release);
        m_widx.fetch_add(count,std::memory_order_release);
        return count;
    }
    size_type acquireReadRegions(size_type count,
            pointer* dataPtr1, size_type* sizePtr1,
            pointer* dataPtr2, size_type* sizePtr2)
    {
        auto ridx = m_ridx.load(std::memory_order_acquire);
        auto widx = m_widx.load(std::memory_order_acquire);
        if((count = std::min<size_type>(count, (widx-ridx)&bigMask))){
            auto roff = ridx & smallMask;
            auto size1 = std::min<size_type>(count, bufferSize - roff);
           *sizePtr1 = size1;
           *dataPtr1 = reinterpret_cast<T*>(&m_data[roff]);
           *sizePtr2 = count - size1;
           *dataPtr2 = reinterpret_cast<T*>(&m_data[0]);
            std::atomic_thread_fence(std::memory_order_acquire);
        }else{
            *dataPtr1 = *dataPtr2 = nullptr;
            *sizePtr1 = *sizePtr2 = 0;
        }
        return count;
    }
    size_type releaseReadRegions(size_type count)
    {
        auto ridx = m_ridx.load(std::memory_order_relaxed);
        auto widx = m_widx.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        count = std::min<size_type>(count, (widx-ridx)&bigMask);
        if(count){
            for(auto idx = ridx; idx < difference_type(ridx + count); ++idx)
                t_cast(m_data[idx&smallMask]).~T();
            std::atomic_thread_fence(std::memory_order_release);
            m_ridx.fetch_add(count,std::memory_order_relaxed);
        }
        return count;
    }
    size_type flushReadData(size_type count)
    {
        return releaseReadRegions(count);
    }
    void clear()
    {
        while(!empty()){
            flushReadData(readAvailable());
        }
    }
    void reset(difference_type diff = 0)
    {
        clear();
        m_ridx = m_widx = diff;
    }
    difference_type read_index() const
    {
        return m_ridx.load(std::memory_order_acquire);
    }
    difference_type write_index() const
    {
        return m_widx.load(std::memory_order_acquire);
    }
};
}
template<class T>
class fifo : public detail::fifo<T, std::is_trivial<T>::value> {
public:
    using super = detail::fifo<T,std::is_trivial<T>::value >;
    using super::super;
};
template<class T>
using FIFO = fifo<T>;

using ring_buffer_size_t = std::size_t;

// MessagePipe represents one side of a TwoWayMessagePipe. The direction of the
// pipe is with respect to the owner so send and recv are
// perspective-dependent. If serializeWrites is true then calls to writeMessages
// will be serialized with a mutex.
template <class SendType, class RecvType>
class MessagePipe {
  public:
    using size_type  = std::size_t;
    using difference_type = std::ptrdiff_t;
    MessagePipe(fifo<SendType>& recv_messages,
                fifo<RecvType>& send_messages,
                std::shared_ptr<void> pTwoWayMessagePipeReference,
                bool serialize_writes)
            : m_recv_messages(recv_messages),
              m_send_messages(send_messages),
              m_pTwoWayMessagePipeReference(pTwoWayMessagePipeReference),
              m_bSerializeWrites(serialize_writes)
    { }
    // Returns the number of RecvType messages waiting to be read by
    // the recv. Non-blocking.
    size_type messageCount() const
    {
        return m_send_messages.readAvailable();
    }
    size_type writeSpace() const
    {
        return m_recv_messages.writeAvailable();
    }
    bool full() const { return m_recv_messages.full();}
    bool empty()const { return m_send_messages.empty(); }

    RecvType readMessage()
    {
        auto _empty = empty();
        auto item = _empty ? RecvType{} : m_send_messages.front();
        if(!_empty)
            m_send_messages.pop_front();
        return item;
    }
    void writeMessage( const SendType & item)
    {
        if(!full())
            m_recv_messages.push_back(item);
    }
    void writeMessage( SendType && item)
    {
        if(!full())
            m_recv_messages.push_back(std::forward<SendType>(item));
    }
    template<class... Args>
    void emplaceMessage(Args && ...args)
    {
        if(!full())
            m_recv_messages.emplace_back(std::forward<Args>(args)...);
    }
    // Read a RecvType written by the recv addressed to the
    // send. Non-blocking.
    size_type readMessages(RecvType* messages, size_type count)
    {
        return m_send_messages.read(messages, count);
    }
    // Writes up to 'count' messages from the 'message' array to the recv
    // and returns the number of successfully written messages. If
    // serializeWrites is active, this method is blocking.
    size_type writeMessages(const SendType* messages, size_type count)
    {
        if (m_bSerializeWrites) m_serializationMutex.lock();
        auto result = m_recv_messages.write(messages, count);
        if (m_bSerializeWrites) m_serializationMutex.unlock();
        return result;
    }
  private:
    QMutex m_serializationMutex;
    fifo<SendType>& m_recv_messages;
    fifo<RecvType>& m_send_messages;
    std::shared_ptr<void> m_pTwoWayMessagePipeReference;
    bool m_bSerializeWrites;
};

// TwoWayMessagePipe is a bare-bones wrapper around the above FIFO class that
// facilitates non-blocking two-way communication. To keep terminology clear,
// there are two sides to the message pipe, the send side and the recv
// side. The non-blocking aspect of the underlying FIFO class requires that the
// send methods and target methods each only be called from a single thread,
// or alternatively guarded with a mutex. The most common use-case of this class
// is sending and receiving messages with the callback thread without the
// callback thread blocking.
//
// This class is an implementation detail and cannot be instantiated
// directly. Use makeTwoWayMessagePipe(...) to create a two-way pipe.
template <class SendType, class RecvType>
class TwoWayMessagePipe {
  public:
    // Creates a TwoWayMessagePipe with SendType and
    // RecvType as the message types. Returns a pair of MessagePipes,
    // the first is the send's pipe (sends SendType and receives
    // RecvType messages) and the second is the recv's pipe
    // (sends RecvType and receives SendType messages).

    static std::pair<
        MessagePipe<SendType, RecvType>*,
        MessagePipe<RecvType, SendType>*>
    makeTwoWayMessagePipe(
                     std::size_t  send_fifo_size,
                     std::size_t recv_fifo_size,
                     bool serialize_send_writes,
                     bool serialize_recv_writes)
    {
        auto pipe = create(send_fifo_size,recv_fifo_size);
        return std::make_pair(
            new MessagePipe<SendType, RecvType>(
                pipe->m_recv_messages, pipe->m_send_messages,
                pipe, serialize_send_writes),
            new MessagePipe<RecvType, SendType>(
                pipe->m_send_messages, pipe->m_recv_messages,
                pipe, serialize_recv_writes)
            );
    }
  private:
    static std::shared_ptr<TwoWayMessagePipe> create(std::size_t sfifo_size, std::size_t rfifo_size)
    {
        return std::shared_ptr<TwoWayMessagePipe>(new TwoWayMessagePipe(sfifo_size,rfifo_size));
    }
    TwoWayMessagePipe(std::size_t send_fifo_size, std::size_t recv_fifo_size)
            : m_recv_messages(recv_fifo_size),
              m_send_messages(send_fifo_size)
    { }
    // Messages waiting to be delivered to the recv.
    fifo<SendType> m_recv_messages;
    // Messages waiting to be delivered to the send.
    fifo<RecvType> m_send_messages;
};
#endif /* FIFO_H */
