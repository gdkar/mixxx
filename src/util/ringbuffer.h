_Pragma("once")

#include <atomic>
#include <memory>
#include <algorithm>
#include <utility>

#include "util/math.h"
template<class T>
class PaUtilRingBuffer {
  long                 bufferSize = 0;
  long                 bigMask    = 0;
  long                 smallMask  = 0;
  std::unique_ptr<T[]> buffer {nullptr};
  std::atomic<long> writeIndex { 0 };
  std::atomic<long> readIndex { 0 };
public: 
  explicit PaUtilRingBuffer ( long buffer_size )
  : bufferSize ( roundUpToPowerOf2(buffer_size ) )
  , bigMask    ( 2 * bufferSize - 1 )
  , smallMask  ( bufferSize - 1 )
  , buffer     ( std::make_unique<T[]> ( bufferSize ) )
  {}
  virtual ~PaUtilRingBuffer ( ) = default;
  virtual long getReadAvailable () const { return ( writeIndex.load() - readIndex.load() ) & bigMask ;}
  virtual long getWriteAvailable() const { return ( bufferSize - getReadAvailable () );}
  virtual void flush () { writeIndex.store ( 0 ); readIndex.store ( 0 ); }
  virtual long getWriteRegions   ( long elementCount, T **pData0, long *pSize0, T **pData1, long *pSize1 )
  {
    auto widx = writeIndex.load();
    auto ridx = readIndex.load();
    auto available = bufferSize - ((widx-ridx)&bigMask);
    elementCount = std::min ( elementCount, available );
    auto index = widx & smallMask;
    if ( index + elementCount > bufferSize )
    {
      auto firstHalf = bufferSize - index;
      *pData0 = (&buffer[index]);
      *pSize0 = firstHalf;
      *pData1 = (&buffer[0]);
      *pSize1 = elementCount - firstHalf;
    }
    else
    {
      *pData0 = (&buffer[index]);
      *pSize0 = elementCount;
      *pData1 = nullptr;
      *pSize1 = 0;
    }
    return elementCount;
  }
  virtual long advanceWriteIndex ( long elementCount )
  {
    return (writeIndex.fetch_add ( elementCount ) + elementCount) & bigMask;
  }
  virtual long getReadRegions    ( long elementCount, T ** pData0, long *pSize0, T **pData1, long *pSize1 )
  {
    auto widx = writeIndex.load ();
    auto ridx = readIndex .load ();
    auto available = (widx-ridx) & bigMask;
    elementCount = std::min(elementCount,available);
    auto index = ridx & smallMask;
    if ( index + elementCount > bufferSize )
    {
      auto firstHalf = bufferSize - index;
      *pData0 = (&buffer[index]);
      *pSize0 = firstHalf;
      *pData1 = (&buffer[0]);
      *pSize1 = elementCount - firstHalf;
    }
    else
    {
      *pData0 = (&buffer[index]);
      *pSize0 = elementCount;
      *pData1 = nullptr;
      *pSize1 = 0;
    }
    return elementCount;
  }
  virtual long advanceReadIndex  ( long elementCount )
  {
    return (readIndex.fetch_add ( elementCount ) + elementCount ) & bigMask;
  }
  virtual long write ( const T *data, long elementCount )
  {
    auto size0 = long{}, size1 = long{};
    T *data0, *data1;
    auto numWritten = getWriteRegions ( elementCount, &data0, &size0, &data1, &size1 );
    std::move ( data, data+size0, data0 );
    data += size0;
    if ( size1 > 0 )
    {
      std::move ( data, data+size1, data1 );
    }
    advanceWriteIndex ( numWritten );
    return numWritten;
  }
  virtual long read  (       T *data, long elementCount )
  {
    auto size0 = long{},size1 = long{};
    T *data0, *data1;
    auto numRead = getReadRegions ( elementCount, &data0, &size0, &data1, &size1 );
    std::move ( data0, data0+size0, data );
    data += size0;
    if ( size1 )
    {
      std::move ( data1, data1+size1, data );
    }
    advanceReadIndex ( numRead );
    return numRead;
  }
};
