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
#include "util/class.h"
#include "util/math.h"

template <class T>
class FIFO {
  public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::int64_t;
    using reference       = T&;
    using pointer         = T*;

  protected:
    size_type        bufferSize;
    difference_type  bigMask;
    difference_type  smallMask;
    std::unique_ptr<T[]> m_data{};
    std::atomic<difference_type> readIndex;
    std::atomic<difference_type> writeIndex;

  public:
    explicit FIFO(size_type size)
    : bufferSize(roundUpToPowerOf2(size))
    , bigMask   ((bufferSize * 2) - 1)
    , smallMask ( bufferSize - 1)
    , m_data    ( std::make_unique<T[]>(bufferSize))
    { }
    virtual ~FIFO() = default;
    size_type capacity() const
    {
        return bufferSize;
    }
    size_type readAvailable() const
    {
        auto ridx = readIndex.load(std::memory_order_acquire);
        auto widx = writeIndex.load(std::memory_order_acquire);
        return (widx - ridx) & bigMask;
    }
    size_type writeAvailable() const
    {
        return bufferSize - readAvailable();
    }
    size_type read(T* pData, size_type count)
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
    size_type write(const T* pData, size_type count)
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
    void writeBlocking(const T* pData, size_type count) {
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
        auto ridx = readIndex.load(std::memory_order_acquire);
        auto widx = writeIndex.load(std::memory_order_acquire);
        count = std::min<size_type>(count, bufferSize - ((widx-ridx)&bigMask));
        auto woff = widx & smallMask;
        auto size1 = std::min<size_type>(count, bufferSize - woff);
        *sizePtr1 = size1;
        *dataPtr1 = &m_data[woff];
        *sizePtr2 = count - size1;
        *dataPtr2 = &m_data[0];
        if(count)
            std::atomic_thread_fence(std::memory_order_acquire);
        return count;
    }
    size_type releaseWriteRegions(size_type count)
    {
        auto ridx = readIndex.load(std::memory_order_acquire);
        auto widx = writeIndex.load(std::memory_order_acquire);
        count = std::min<size_type>(count, bufferSize - ((widx-ridx)&bigMask));
        std::atomic_thread_fence(std::memory_order_release);
        writeIndex.fetch_add(count,std::memory_order_release);
        return count;
    }
    size_type acquireReadRegions(size_type count,
            pointer* dataPtr1, size_type* sizePtr1,
            pointer* dataPtr2, size_type* sizePtr2)
    {
        auto ridx = readIndex.load(std::memory_order_acquire);
        auto widx = writeIndex.load(std::memory_order_acquire);
        count = std::min<size_type>(count, (widx-ridx)&bigMask);
        auto roff = ridx & smallMask;
        auto size1 = std::min<size_type>(count, bufferSize - roff);
        *sizePtr1 = size1;
        *dataPtr1 = &m_data[roff];
        *sizePtr2 = count - size1;
        *dataPtr2 = &m_data[0];
        if(count)
            std::atomic_thread_fence(std::memory_order_acquire);
        return count;
    }
    size_type releaseReadRegions(size_type count)
    {
        auto ridx = readIndex.load(std::memory_order_acquire);
        auto widx = writeIndex.load(std::memory_order_acquire);
        count = std::min<size_type>(count, (widx-ridx)&bigMask);
        std::atomic_thread_fence(std::memory_order_release);
        readIndex.fetch_add(count,std::memory_order_release);
        return count;
    }
    size_type flushReadData(size_type count)
    {
        return releaseReadRegions(count);
    }
};

// MessagePipe represents one side of a TwoWayMessagePipe. The direction of the
// pipe is with respect to the owner so sender and receiver are
// perspective-dependent. If serializeWrites is true then calls to writeMessages
// will be serialized with a mutex.
template <class SenderMessageType, class ReceiverMessageType>
class MessagePipe {
  public:
    using size_type  = std::size_t;
    using difference_type = std::ptrdiff_t;

    MessagePipe(FIFO<SenderMessageType>& receiver_messages,
                FIFO<ReceiverMessageType>& sender_messages,
                std::shared_ptr<void> pTwoWayMessagePipeReference,
                bool serialize_writes)
            : m_receiver_messages(receiver_messages),
              m_sender_messages(sender_messages),
              m_pTwoWayMessagePipeReference(pTwoWayMessagePipeReference),
              m_bSerializeWrites(serialize_writes) {
    }
    // Returns the number of ReceiverMessageType messages waiting to be read by
    // the receiver. Non-blocking.
    size_type messageCount() const {
        return m_sender_messages.readAvailable();
    }
    // Read a ReceiverMessageType written by the receiver addressed to the
    // sender. Non-blocking.
    size_type readMessages(ReceiverMessageType* messages, size_type count) {
        return m_sender_messages.read(messages, count);
    }
    // Writes up to 'count' messages from the 'message' array to the receiver
    // and returns the number of successfully written messages. If
    // serializeWrites is active, this method is blocking.
    size_type writeMessages(const SenderMessageType* messages, size_type count)
    {
        if (m_bSerializeWrites) m_serializationMutex.lock();
        auto result = m_receiver_messages.write(messages, count);
        if (m_bSerializeWrites) m_serializationMutex.unlock();
        return result;
    }
  private:
    QMutex m_serializationMutex;
    FIFO<SenderMessageType>& m_receiver_messages;
    FIFO<ReceiverMessageType>& m_sender_messages;
    std::shared_ptr<void> m_pTwoWayMessagePipeReference;
    bool m_bSerializeWrites;
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

    static std::pair<MessagePipe<SenderMessageType, ReceiverMessageType>*,
                 MessagePipe<ReceiverMessageType, SenderMessageType>*> makeTwoWayMessagePipe(
                     std::size_t  sender_fifo_size,
                     std::size_t receiver_fifo_size,
                     bool serialize_sender_writes,
                     bool serialize_receiver_writes) {
        auto pipe = create(sender_fifo_size,receiver_fifo_size);
        return std::make_pair(new MessagePipe<SenderMessageType, ReceiverMessageType>(
                             pipe->m_receiver_messages, pipe->m_sender_messages,
                             pipe,
                             serialize_sender_writes),
                         new MessagePipe<ReceiverMessageType, SenderMessageType>(
                             pipe->m_sender_messages, pipe->m_receiver_messages,
                             pipe,
                             serialize_receiver_writes));
    }

  private:
    static std::shared_ptr<TwoWayMessagePipe> create(std::size_t sfifo_size, std::size_t rfifo_size)
    {
        return std::shared_ptr<TwoWayMessagePipe>(new TwoWayMessagePipe(sfifo_size,rfifo_size));
    }
    TwoWayMessagePipe(std::size_t sender_fifo_size, std::size_t receiver_fifo_size)
            : m_receiver_messages(receiver_fifo_size),
              m_sender_messages(sender_fifo_size)
    { }
    // Messages waiting to be delivered to the receiver.
    FIFO<SenderMessageType> m_receiver_messages;
    // Messages waiting to be delivered to the sender.
    FIFO<ReceiverMessageType> m_sender_messages;
};
#endif /* FIFO_H */
