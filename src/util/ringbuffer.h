_Pragma("once")

#include <atomic>
#include <memory>
#include <algorithm>
#include <utility>
#include <cstddef>
#include "util/math.h"
template<class T>
class PaUtilRingBuffer {
  int64_t                 bufferSize = 0;
  int64_t                 bigMask    = 0;
  int64_t                 smallMask  = 0;
  std::unique_ptr<T[]> buffer {nullptr};
  std::atomic<int64_t> writeIndex { 0 };
  std::atomic<int64_t> readIndex { 0 };
public: 
  explicit PaUtilRingBuffer ( int64_t buffer_size )
  : bufferSize ( roundUpToPowerOf2(buffer_size ) )
  , bigMask    ( 2 * bufferSize - 1 )
  , smallMask  ( bufferSize - 1 )
  , buffer     ( std::make_unique<T[]> ( bufferSize ) )
  {}
  virtual ~PaUtilRingBuffer ( ) = default;
  virtual int64_t getReadAvailable () const { return ( (writeIndex.load()&bigMask) - (readIndex.load()&bigMask) ) & bigMask ;}
  virtual int64_t getWriteAvailable() const { return ( bufferSize - getReadAvailable () );}
  virtual void flush () { writeIndex.store ( 0 ); readIndex.store ( 0 ); }
  virtual int64_t getWriteRegions   ( int64_t elementCount, T **pData0, int64_t *pSize0, T **pData1, int64_t *pSize1 )
  {
    auto widx = writeIndex.load() & bigMask;
    auto ridx = readIndex.load()  & bigMask;
    auto available = bufferSize - ((widx-ridx)&bigMask);
    if((elementCount = std::min ( elementCount, available )))
    {
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
    }
    return elementCount;
  }
  virtual int64_t advanceWriteIndex ( int64_t elementCount )
  {
    auto widx = writeIndex.load() & bigMask;
    auto ridx = readIndex.load() & bigMask;
    auto available = bufferSize - ((widx-ridx)&bigMask);
    if((elementCount = std::min ( elementCount, available )))
    writeIndex.fetch_add ( elementCount );
    return elementCount;
  }
  virtual int64_t getReadRegions    ( int64_t elementCount, T ** pData0, int64_t *pSize0, T **pData1, int64_t *pSize1 )
  {
    auto widx = writeIndex.load () & bigMask;
    auto ridx = readIndex .load () & bigMask;
    auto available = (widx-ridx) & bigMask;
    if((elementCount = std::min(elementCount,available)))
    {
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
    }
    return elementCount;
  }
  virtual int64_t advanceReadIndex  ( int64_t elementCount )
  {
    auto widx = writeIndex.load () & bigMask;
    auto ridx = readIndex .load () & bigMask;
    auto available = (widx-ridx) & bigMask;
    if((elementCount = std::min(elementCount,available)))
      readIndex.fetch_add ( elementCount );
    return elementCount;
  }
  virtual int64_t write ( const T *data, int64_t elementCount )
  {
    auto size0 = int64_t{}, size1 = int64_t{};
    T *data0, *data1;
    auto numWritten = getWriteRegions ( elementCount, &data0, &size0, &data1, &size1 );
    std::move ( data, data+size0, data0 );
    data += size0;
    if ( size1 > 0 ) std::move ( data, data+size1, data1 );
    advanceWriteIndex ( numWritten );
    return numWritten;
  }
  virtual int64_t read  (       T *data, int64_t elementCount )
  {
    auto size0 = int64_t{},size1 = int64_t{};
    T *data0, *data1;
    auto numRead = getReadRegions ( elementCount, &data0, &size0, &data1, &size1 );
    std::move ( data0, data0+size0, data );
    data += size0;
    if ( size1 ) std::move ( data1, data1+size1, data );
    advanceReadIndex ( numRead );
    return numRead;
  }
};
