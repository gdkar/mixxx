_Pragma("once")

#include <atomic>
#include <memory>
#include <algorithm>
#include <utility>
#include <cstddef>
#include "util/math.h"
template<class T>
class PaUtilRingBuffer {
public:
  using size_type       = uint64_t;
  using difference_type = int64_t;
private:
  size_type                bufferSize = 0;
  size_type                bigMask    = 0;
  size_type                smallMask  = 0;
  std::unique_ptr<T[]> buffer {nullptr};
  std::atomic<difference_type> writeIndex { 0 };
  std::atomic<difference_type> readIndex { 0 };
public: 
  explicit PaUtilRingBuffer ( size_type buffer_size )
  : bufferSize ( roundUpToPowerOf2(buffer_size ) )
  , bigMask    ( 2 * bufferSize - 1 )
  , smallMask  ( bufferSize - 1 )
  , buffer     ( std::make_unique<T[]> ( bufferSize ) )
  {}
  virtual ~PaUtilRingBuffer ( ) = default;
  virtual size_type getReadAvailable () const 
  { 
    return ( (writeIndex.load()&bigMask) - (readIndex.load()&bigMask) ) & bigMask ;
  }
  virtual size_type  getWriteAvailable() const 
  { 
    return ( bufferSize - getReadAvailable () );
  }
  virtual void flush () { writeIndex.store ( 0 ); readIndex.store ( 0 ); }
  virtual size_type getWriteRegions   ( size_type elementCount, T **pData0, size_type *pSize0, T **pData1, size_type *pSize1 )
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
  virtual size_type advanceWriteIndex ( size_type elementCount )
  {
    auto widx = writeIndex.load() & bigMask;
    auto ridx = readIndex.load() & bigMask;
    auto available = bufferSize - ((widx-ridx)&bigMask);
    if((elementCount = std::min ( elementCount, available )))
    writeIndex.fetch_add ( elementCount );
    return elementCount;
  }
  virtual size_type getReadRegions    ( size_type elementCount, T ** pData0, size_type *pSize0, T **pData1, size_type *pSize1 )
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
  virtual size_type advanceReadIndex  ( size_type elementCount )
  {
    auto widx = writeIndex.load () & bigMask;
    auto ridx = readIndex .load () & bigMask;
    auto available = (widx-ridx) & bigMask;
    if((elementCount = std::min(elementCount,available)))
      readIndex.fetch_add ( elementCount );
    return elementCount;
  }
  virtual size_type write ( const T *data, size_type elementCount )
  {
    auto size0 = size_type {}, size1 = size_type {};
    T *data0, *data1;
    auto numWritten = getWriteRegions ( elementCount, &data0, &size0, &data1, &size1 );
    std::move ( data, data+size0, data0 );
    data += size0;
    if ( size1 > 0 ) std::move ( data, data+size1, data1 );
    advanceWriteIndex ( numWritten );
    return numWritten;
  }
  virtual size_type read  (       T *data, size_type elementCount )
  {
    auto size0 = size_type {},size1 = size_type {};
    T *data0, *data1;
    auto numRead = getReadRegions ( elementCount, &data0, &size0, &data1, &size1 );
    std::move ( data0, data0+size0, data );
    data += size0;
    if ( size1 ) std::move ( data1, data1+size1, data );
    advanceReadIndex ( numRead );
    return numRead;
  }
};
