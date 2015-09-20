_Pragma("once")
#include <QtDebug>
#include <QMutex>
#include <QScopedPointer>
#include <QSemaphore>
#include <QSharedPointer>

#include "util/ringbuffer.h"
#include "util/reference.h"
#include "util/math.h"
#include "util.h"

template <class DataType>
class FIFO : public PaUtilRingBuffer<DataType>{
  QSemaphore m_spaceAvailable { 0 };
  public:
    using PaUtilRingBuffer<DataType>::getReadAvailable;
    using PaUtilRingBuffer<DataType>::getWriteAvailable;
    using PaUtilRingBuffer<DataType>::getReadRegions;
    using PaUtilRingBuffer<DataType>::getWriteRegions;
    using PaUtilRingBuffer<DataType>::advanceReadIndex;
    using PaUtilRingBuffer<DataType>::advanceWriteIndex;
    FIFO ( const FIFO& ) = delete;
    FIFO ( FIFO && ) = default;
    FIFO&operator = ( const FIFO& ) = delete;
    FIFO&operator = ( FIFO && ) = default;
    explicit FIFO(long size)
            : PaUtilRingBuffer<DataType> ( size ) 
    { 
      m_spaceAvailable.release ( getWriteAvailable () );
    }
    virtual ~FIFO() = default;
    long readAvailable() const 
    { 
      return getReadAvailable () ; 
    }
    long writeAvailable() const 
    { 
      return getWriteAvailable () ; 
    }
    long read ( DataType *pData, long count ) override {
      auto n = PaUtilRingBuffer<DataType>::read (pData, count);
      m_spaceAvailable.release(n);
      return n;
    }
    long write( const DataType *pData, long count ) override {
      auto n = PaUtilRingBuffer<DataType>::write(pData, count);
      m_spaceAvailable.tryAcquire(n);
      return n;
    }
    void writeBlocking(const DataType* pData, long count) {
        auto written = long{0};
        if ( count )
        {
          m_spaceAvailable.acquire ( 1 );
          while ( written < count )
          {
            auto n = PaUtilRingBuffer<DataType>::write ( pData + written, count - written );
            written += n;
            if ( written < count ) m_spaceAvailable.acquire ( n );
            else                   m_spaceAvailable.acquire ( n - 1 );
          }
        }
    }
    long aquireWriteRegions(long count, DataType** dataPtr0, long* sizePtr0, DataType** dataPtr1, long* sizePtr1)
    {
        return getWriteRegions ( count, dataPtr0, sizePtr0, dataPtr1, sizePtr1 );
    }
    long releaseWriteRegions(long count) 
    {
        auto n = advanceWriteIndex ( count );
        m_spaceAvailable.tryAcquire ( count);
        return n;
    }
    long aquireReadRegions(long count, DataType** dataPtr0, long * sizePtr0, DataType** dataPtr1, long * sizePtr1)
    {
      return getReadRegions ( count, dataPtr0, sizePtr0, dataPtr1, sizePtr1 );
    }
    long releaseReadRegions(long count)
    {
      auto n =  advanceReadIndex ( count );
      m_spaceAvailable.release ( count );
      return n;
    }
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
    long messageCount() const 
    {
      return m_sender_messages.readAvailable();
    }
    // Read a ReceiverMessageType written by the receiver addressed to the
    // sender. Non-blocking.
    long readMessages(ReceiverMessageType* messages, long count) 
    {
      return m_sender_messages.read(messages, count);
    }
    // Writes up to 'count' messages from the 'message' array to the receiver
    // and returns the number of successfully written messages. If
    // serializeWrites is active, this method is blocking.
    long writeMessages(const SenderMessageType* messages, long count) {
        if (m_bSerializeWrites) { m_serializationMutex.lock(); }
        auto result = m_receiver_messages.write(messages, count);
        if (m_bSerializeWrites) { m_serializationMutex.unlock(); }
        return result;
    }
  private:
    QMutex m_serializationMutex;
    FIFO<SenderMessageType>& m_receiver_messages;
    FIFO<ReceiverMessageType>& m_sender_messages;
    QScopedPointer<BaseReferenceHolder> m_pTwoWayMessagePipeReference;
    bool m_bSerializeWrites = false;
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
                     long sender_fifo_size,
                     long receiver_fifo_size,
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
    virtual ~TwoWayMessagePipe() = default;
  private:
    TwoWayMessagePipe(long sender_fifo_size, long receiver_fifo_size)
            : m_receiver_messages(receiver_fifo_size),
              m_sender_messages(sender_fifo_size) {
    }
    // Messages waiting to be delivered to the receiver.
    FIFO<SenderMessageType> m_receiver_messages;
    // Messages waiting to be delivered to the sender.
    FIFO<ReceiverMessageType> m_sender_messages;
    // This #define is because the macro gets confused by the template
    // parameters.
};
