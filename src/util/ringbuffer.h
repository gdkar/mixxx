_Pragma("once")

#include <atomic>
#include <memory>
#include <algorithm>
#include <utility>
#include <cstddef>
#include <cmath>

namespace{
  template<typename T>
  constexpr T nextPow2(T arg)
  {
    arg--;
    for(auto shift = size_t{1}; shift < (8 * sizeof(T)); shift<<=1)
    {
      arg |= (arg>>shift);
    }
    return arg+1;
  }
}
template<class T>
class PaUtilRingBuffer {
  using size_type = int64_t;
  size_type                 bufferSize = 0;
  size_type                 bigMask    = 0;
  size_type                 smallMask  = 0;
  std::unique_ptr<T[]> buffer {nullptr};
  std::atomic<size_type> writeIndex { 0 };
  std::atomic<size_type> readIndex  { 0 };
public:
  explicit PaUtilRingBuffer ( size_type buffer_size );
  virtual ~PaUtilRingBuffer ( );
  virtual void flush ();
  virtual size_type getReadAvailable () const;
  virtual size_type getWriteAvailable() const;
  virtual size_type getWriteRegions (
        size_type elementCount
      , T ** pData0
      , size_type *pSize0
      , T **pData1
      , size_type *pSize1
    );
  virtual size_type getReadRegions (
        size_type elementCount
      , T ** pData0
      , size_type *pSize0
      , T **pData1
      , size_type *pSize1
    );
  virtual size_type advanceWriteIndex (size_type elementCount     );
  virtual size_type advanceReadIndex  ( size_type elementCount    );
  virtual size_type write             ( const T *data, size_type elementCount );
  virtual size_type read              (       T *data, size_type elementCount );
};

template<typename T>
PaUtilRingBuffer<T>::PaUtilRingBuffer ( size_type buffer_size )
  : bufferSize ( nextPow2( buffer_size ) )
  , bigMask    ( 2 * bufferSize - 1 )
  , smallMask  ( bufferSize - 1 )
  , buffer     ( std::make_unique<T[]> ( bufferSize ) )
{
}
template<typename T>
PaUtilRingBuffer<T>::~PaUtilRingBuffer() = default;
template<typename T>
void PaUtilRingBuffer<T>::flush ()
{
  readIndex.store ( writeIndex.load() );
}
template<typename T>
typename PaUtilRingBuffer<T>::size_type PaUtilRingBuffer<T>::getReadAvailable () const
{
  return ( (writeIndex.load()&bigMask) - (readIndex.load()&bigMask) ) & bigMask ;
}
template<typename T>
typename PaUtilRingBuffer<T>::size_type PaUtilRingBuffer<T>::getWriteAvailable() const
{
  return ( std::max(size_type(0),bufferSize - getReadAvailable () ) );
}
template<typename T>
typename PaUtilRingBuffer<T>::size_type PaUtilRingBuffer<T>::getWriteRegions(
      size_type elementCount
    , T **pData0
    , size_type *pSize0
    , T **pData1
    , size_type *pSize1
  )
{
  auto widx = writeIndex.load() & bigMask;
  auto ridx = readIndex.load()  & bigMask;
  auto available = bufferSize - ((widx-ridx)&bigMask);
  if((elementCount = std::max( size_type{0},std::min ( elementCount, available ))))
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
template<typename T>
typename PaUtilRingBuffer<T>::size_type PaUtilRingBuffer<T>::advanceWriteIndex (
    size_type elementCount
    )
{
  auto widx = writeIndex.load () & bigMask;
  auto ridx = readIndex .load () & bigMask;
  auto available = (widx-ridx) & bigMask;
  if((elementCount = std::max(size_type{0},std::min(elementCount,available))))
    writeIndex.fetch_add ( elementCount );
  return elementCount;
}
template<typename T>
typename PaUtilRingBuffer<T>::size_type PaUtilRingBuffer<T>::getReadRegions (
    size_type elementCount,
    T ** pData0,
    size_type *pSize0,
    T **pData1,
    size_type *pSize1
    )
  {
    auto widx = writeIndex.load () & bigMask;
    auto ridx = readIndex .load () & bigMask;
    auto available = (widx-ridx) & bigMask;
    if((elementCount = std::max(size_type{0},std::min(elementCount,available))))
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
template<typename T>
typename PaUtilRingBuffer<T>::size_type PaUtilRingBuffer<T>::advanceReadIndex  ( size_type elementCount )
{
  auto widx = writeIndex.load () & bigMask;
  auto ridx = readIndex .load () & bigMask;
  auto available = bufferSize - ((widx-ridx) & bigMask);
  if((elementCount = std::max(size_type{0},std::min(elementCount,available))))
    readIndex.fetch_add ( elementCount );
  return elementCount;
}
template<typename T>
typename PaUtilRingBuffer<T>::size_type PaUtilRingBuffer<T>::write ( const T *data, size_type elementCount )
{
  auto size0 = size_type{}, size1 = size_type{};
  T *data0, *data1;
  auto numWritten = getWriteRegions ( elementCount, &data0, &size0, &data1, &size1 );
  std::move ( data, data+size0, data0 );
  data += size0;
  if ( size1 > 0 ) std::move ( data, data+size1, data1 );
  advanceWriteIndex ( numWritten );
  return numWritten;
}
template<typename T>
typename PaUtilRingBuffer<T>::size_type PaUtilRingBuffer<T>::read  (       T *data, size_type elementCount )
{
  auto size0 = size_type{},size1 = size_type{};
  T *data0, *data1;
  auto numRead = getReadRegions ( elementCount, &data0, &size0, &data1, &size1 );
  std::move ( data0, data0+size0, data );
  data += size0;
  if ( size1 ) std::move ( data1, data1+size1, data );
  advanceReadIndex ( numRead );
  return numRead;
}
