#ifndef FIFO_H
#define FIFO_H

#include <QtDebug>
#include <QMutex>
#include <QThread>
#include <QScopedPointer>
#include <QSharedPointer>
#include <atomic>
#include <memory>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>
#include <type_traits>

#include "util/class.h"
#include "util/math.h"
#include "util/reference.h"
#include "util/semaphore.hpp"

template<class T>
class FIFO {
protected:
    int64_t                 m_size{0};
    int64_t                 m_mask{0};
    std::atomic<int64_t>    m_ridx{0};
    std::atomic<int64_t>    m_widx{0};
    std::unique_ptr<T[]>    m_data{};
public:
    using value_type      = T;
    using size_type       = int;
    using difference_type = int64_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    static constexpr const size_type npos = std::numeric_limits<size_type>::max() / 8;
    FIFO() = default;
    FIFO(FIFO && o) noexcept
      : m_size(std::exchange(o.m_size,0))
      , m_mask(std::exchange(o.m_mask,0))
      , m_ridx(o.m_ridx.exchange(0))
      , m_widx(o.m_widx.exchange(0))
      , m_data(std::move(o.m_data)) { }
    FIFO&operator = (FIFO && o) noexcept
    {
       m_size=std::exchange(o.m_size,0);
       m_mask=std::exchange(o.m_mask,0);
       m_ridx=o.m_ridx.exchange(0);
       m_widx=o.m_widx.exchange(0);
       m_data.swap(o.m_data);
    }
    FIFO(size_type _size)
    : m_size(roundUpToPowerOf2(_size))
    , m_mask(m_size - 1)
    , m_data(std::make_unique<T[]>(m_size)){}
   ~FIFO() = default;
    size_type readAvailable() const
    {
        auto widx = m_widx.load();auto ridx = m_ridx.load();
        return widx - ridx;
    }
    size_type writeAvailable() const
    {
        auto widx = m_widx.load();auto ridx = m_ridx.load();
        return m_size - (widx - ridx);
    }
    bool full() const  { return (m_ridx.load() + m_size) == m_widx.load();}
    bool empty() const { return m_ridx.load() == m_widx.load();}
    std::pair<T*, T*> read_contig(size_type count = npos,size_type offset = 0)
    {
        auto widx = m_widx.load();auto ridx = m_ridx.load() + offset;
        if(offset < 0 || widx - ridx < 0)
            return std::make_pair(static_cast<T*>(nullptr),0);
        auto rpos = ridx & m_mask;
        count     = std::min<size_type>(count,std::min<difference_type>(m_size - rpos, widx - ridx));
        return std::make_pair(m_data.get() + rpos, m_data.get() + rpos + count);
    }
    std::pair<pointer, size_type> read_contig_n(size_type count = npos, difference_type offset = 0)
    {
        auto widx = m_widx.load();auto ridx = m_ridx.load() + offset;
        if(offset < 0 || widx - ridx < 0)
            return std::make_pair(static_cast<T*>(nullptr),0);
        auto rpos = ridx & m_mask;
        count     = std::min<size_type>(count,std::min<difference_type>(m_size - rpos, widx - ridx));
        return std::make_pair(m_data.get() + rpos, count);
    }
    std::pair<pointer, pointer> write_contig(size_type count = npos, difference_type offset = 0)
    {
        auto widx = m_widx.load() + offset;auto ridx = m_ridx.load();
        if(offset < 0 || ridx + m_size - widx < 0)
            return std::make_pair(static_cast<T*>(nullptr),0);
        auto wpos = widx & m_mask;
        count     = std::min<size_type>(count,std::min<difference_type>(m_size - wpos, m_size + ridx - widx));
        return std::make_pair(m_data.get() + wpos, m_data.get() + wpos + count);
    }
    std::pair<pointer, size_type> write_contig_n(size_type count = npos, size_type offset = 0)
    {
        auto widx = m_widx.load() + offset;auto ridx = m_ridx.load();
        if(offset < 0 || ridx + m_size - widx < 0)
            return std::make_pair(static_cast<T*>(nullptr),0);
        auto wpos = widx & m_mask;
        count     = std::min<size_type>(count,std::min<difference_type>(m_size - wpos, m_size + ridx - widx));
        return std::make_pair(m_data.get() + wpos, count);
    }
    template<class Iter>
    size_type read(Iter pData, size_type count)
    {
        auto total = count;
        for(auto i = 0; i < 2 && count > 0; i++) {
            auto region = read_contig_n(count);
            pData = std::copy_n(region.first,region.second,pData);
            count -= region.second;
            m_ridx.fetch_add(region.second);
        }
        return total - count;
    }
    template<class Iter>
    size_type write(Iter pData, size_type count)
    {
        auto total = count;
        for(auto i = 0; i < 2 && count > 0; i++) {
            auto region = write_contig_n(count);
            pData = std::copy_n(pData,region.second,region.first);
            count -= region.second;
            m_widx.fetch_add(region.second);
        }
        return total - count;
    }
    size_type get_write_regions(size_type count,
        pointer* dataPtr1, size_type * sizePtr1,
        pointer* dataPtr2, size_type * sizePtr2) {
        size_type s1, s2;
        if(!sizePtr1) sizePtr1 = &s1;
        if(!sizePtr2) sizePtr2 = &s2;
        std::tie(*dataPtr1, *sizePtr1) = write_contig_n(count);
        std::tie(*dataPtr2, *sizePtr2) = write_contig_n(count - *sizePtr1, *sizePtr1);
        return *sizePtr1 + *sizePtr2;
    }
    size_type get_read_regions(size_type  count,
        pointer* dataPtr1, size_type* sizePtr1,
        pointer* dataPtr2, size_type* sizePtr2) {
        size_type s1, s2;
        if(!sizePtr1) sizePtr1 = &s1;
        if(!sizePtr2) sizePtr2 = &s2;
        std::tie(*dataPtr1, *sizePtr1) = read_contig_n(count);
        std::tie(*dataPtr2, *sizePtr2) = read_contig_n(count - *sizePtr1, *sizePtr1);
        return *sizePtr1 + *sizePtr2;
    }
    size_type commit_write(size_type count)
    {
        auto widx = m_widx.load();auto ridx = m_ridx.load();
        count = std::min<size_type>(count, ridx + m_size - widx);
        m_widx.fetch_add(count);
        return count;
    }
    size_type commit_read(size_type count)
    {
        auto widx = m_widx.load();auto ridx = m_ridx.load();
        count = std::min<size_type>(count, widx - ridx);
        m_ridx.fetch_add(count);
        return count;
    }
    template<class Iter>
    void write_blocking(Iter it, size_type count) {
        auto total = 0;
        while(total != count) {
            auto partial = write(it,count - total);
            std::advance(it, partial);
            total += partial;
        }
    }
    void write_blocking(const T&item)
    {
        while(!push_back(item))
            QThread::yieldCurrentThread();
    }
    T &front() { return m_data[m_ridx.load()&m_mask]; }
    const T &front() const { return m_data[m_ridx.load()&m_mask]; }
    T &back() { return m_data[(m_widx.load() - 1) & m_mask]; }
    const T &back() const { return m_data[(m_widx.load() -1)&m_mask]; }
    T &wback() { return m_data[(m_widx.load()) & m_mask]; }
    const T &wback() const { return m_data[(m_widx.load())&m_mask]; }

    void pop_front() { if(!empty()) m_ridx.fetch_add(1);}
    bool push_back(const T &t) { 
        if(full()){
            return false;
        }else{
            wback() = t;
            m_widx.fetch_add(1);
            return true;
        }
    }
    difference_type read_index() const { return m_ridx.load(); }
    difference_type write_index() const { return m_widx.load(); }
    size_type       read_offset() const { return static_cast<size_type>(read_index() & m_mask); }
    size_type       write_offset() const { return static_cast<size_type>(write_index() & m_mask); }
    void reset() { m_ridx.store(0); m_widx.store(0);}
};
// MessagePipe represents one side of a TwoWayMessagePipe. The direction of the
// pipe is with respect to the owner so sender and receiver are
// perspective-dependent. If serializeWrites is true then calls to writeMessages
// will be serialized with a mutex.
template <class SenderMessageType, class ReceiverMessageType>
class MessagePipe {
  public:
    MessagePipe(FIFO<SenderMessageType>& receiver_messages,
                FIFO<ReceiverMessageType>& sender_messages,
                BaseReferenceHolder* pTwoWayMessagePipeReference,
                bool serialize_writes)
            : m_receiver_messages(receiver_messages),
              m_sender_messages(sender_messages),
              m_pTwoWayMessagePipeReference(pTwoWayMessagePipeReference),
              m_bSerializeWrites(serialize_writes) {
    }

    // Returns the number of ReceiverMessageType messages waiting to be read by
    // the receiver. Non-blocking.
    inline int messageCount() const {
        return m_sender_messages.readAvailable();
    }

    // Read a ReceiverMessageType written by the receiver addressed to the
    // sender. Non-blocking.
    inline int readMessages(ReceiverMessageType* messages, int count) {
        return m_sender_messages.read(messages, count);
    }

    // Writes up to 'count' messages from the 'message' array to the receiver
    // and returns the number of successfully written messages. If
    // serializeWrites is active, this method is blocking.
    inline int writeMessages(const SenderMessageType* messages, int count) {
        if (m_bSerializeWrites) {
            m_serializationMutex.lock();
        }
        int result = m_receiver_messages.write(messages, count);
        if (m_bSerializeWrites) {
            m_serializationMutex.unlock();
        }
        return result;
    }

  private:
    QMutex m_serializationMutex;
    FIFO<SenderMessageType>& m_receiver_messages;
    FIFO<ReceiverMessageType>& m_sender_messages;
    QScopedPointer<BaseReferenceHolder> m_pTwoWayMessagePipeReference;
    bool m_bSerializeWrites;

#define COMMA ,
    DISALLOW_COPY_AND_ASSIGN(MessagePipe<SenderMessageType COMMA ReceiverMessageType>);
#undef COMMA
};

// TwoWayMessagePipe is a bare-bones wrapper around the above FIFO class that
// facilitates non-blocking two-way communication. To keep terminology clear,
// there are two sides to the message pipe, the sender side and the receiver
// side. The non-blocking aspect of the underlying FIFO class requires that the
// sender methods and target methods each only be called from a single thread,
// or alternatively guarded with a mutex. The most common use-case of this class
// is sending and receiving messages with the callback thread without the
// callback thread blocking.
//
// This class is an implementation detail and cannot be instantiated
// directly. Use makeTwoWayMessagePipe(...) to create a two-way pipe.
template <class SenderMessageType, class ReceiverMessageType>
class TwoWayMessagePipe {
  public:
    // Creates a TwoWayMessagePipe with SenderMessageType and
    // ReceiverMessageType as the message types. Returns a pair of MessagePipes,
    // the first is the sender's pipe (sends SenderMessageType and receives
    // ReceiverMessageType messages) and the second is the receiver's pipe
    // (sends ReceiverMessageType and receives SenderMessageType messages).
    static QPair<MessagePipe<SenderMessageType, ReceiverMessageType>*,
                 MessagePipe<ReceiverMessageType, SenderMessageType>*> makeTwoWayMessagePipe(
                     int sender_fifo_size,
                     int receiver_fifo_size,
                     bool serialize_sender_writes,
                     bool serialize_receiver_writes) {
        QSharedPointer<TwoWayMessagePipe<SenderMessageType, ReceiverMessageType> > pipe(
            new TwoWayMessagePipe<SenderMessageType, ReceiverMessageType>(
                sender_fifo_size, receiver_fifo_size));

        return QPair<MessagePipe<SenderMessageType, ReceiverMessageType>*,
                     MessagePipe<ReceiverMessageType, SenderMessageType>*>(
                         new MessagePipe<SenderMessageType, ReceiverMessageType>(
                             pipe->m_receiver_messages, pipe->m_sender_messages,
                             new ReferenceHolder<TwoWayMessagePipe<SenderMessageType, ReceiverMessageType> >(pipe),
                             serialize_sender_writes),
                         new MessagePipe<ReceiverMessageType, SenderMessageType>(
                             pipe->m_sender_messages, pipe->m_receiver_messages,
                             new ReferenceHolder<TwoWayMessagePipe<SenderMessageType, ReceiverMessageType> >(pipe),
                             serialize_receiver_writes));
    }

  private:
    TwoWayMessagePipe(int sender_fifo_size, int receiver_fifo_size)
            : m_receiver_messages(receiver_fifo_size),
              m_sender_messages(sender_fifo_size) {
    }

    // Messages waiting to be delivered to the receiver.
    FIFO<SenderMessageType> m_receiver_messages;
    // Messages waiting to be delivered to the sender.
    FIFO<ReceiverMessageType> m_sender_messages;

    // This #define is because the macro gets confused by the template
    // parameters.
#define COMMA ,
    DISALLOW_COPY_AND_ASSIGN(TwoWayMessagePipe<SenderMessageType COMMA ReceiverMessageType>);
#undef COMMA
};

#endif /* FIFO_H */
