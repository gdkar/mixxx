_Pragma("once")
/*
 * $Id: pa_ringbuffer.h 1734 2011-08-18 11:19:36Z rossb $
 * Portable Audio I/O Library
 * Ring Buffer utility.
 *
 * Author: Phil Burk, http://www.softsynth.com
 * modified for SMP safety on OS X by Bjorn Roche.
 * also allowed for const where possible.
 * modified for multiple-byte-sized data elements by Sven Fischer 
 *
 * Note that this is safe only for a single-thread reader
 * and a single-thread writer.
 *
 * This program is distributed with the PortAudio Portable Audio Library.
 * For more information see: http://www.portaudio.com
 * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

/*
 * Wholely re-written (twice. first using C11 std_atomic.h atomics,
 * and then again in straight C++11 by gabriel d. karpman, c. 2015/7/19
 */

/** @file
 @ingroup common_src
 @brief Single-reader single-writer lock-free ring buffer

 PaUtilRingBuffer is a ring buffer used to transport samples between
 different execution contexts (threads, OS callbacks, interrupt handlers)
 without requiring the use of any locks. This only works when there is
 a single reader and a single writer (ie. one thread or callback writes
 to the ring buffer, another thread or callback reads from it).

 The PaUtilRingBuffer structure manages a ring buffer containing N 
 elements, where N must be a power of two. An element may be any size 
 (specified in bytes).

 The memory area used to store the buffer elements must be allocated by 
 the client prior to calling PaUtil_InitializeRingBuffer() and must outlive
 the use of the ring buffer.
*/




#include <atomic>
#include <memory>
#include <algorithm>
#include <utility>
#include "util/math.h"
template<class T>
class PaUtilRingBuffer{
  public:
    long         bufferSize; /**< Number of elements in FIFO. Power of 2. Set by PaUtil_InitRingBuffer. */
    std::atomic<long> writeIndex; /**< Index of next writable element. Set by PaUtil_AdvanceRingBufferWriteIndex. */
    std::atomic<long> readIndex;  /**< Index of next readable element. Set by PaUtil_AdvanceRingBufferReadIndex. */
    long         bigMask;    /**< Used for wrapping indices with extra bit to distinguish full/empty. */
    long         smallMask;  /**< Used for fitting indices to buffer. */
    std::unique_ptr<T[]> buffer;
    explicit PaUtilRingBuffer(long buffer_size):
      bufferSize(roundUpToPowerOf2(buffer_size)),
      writeIndex(0),
      readIndex(0),
      bigMask((bufferSize*2)-1),
      smallMask(bufferSize-1),
      buffer(std::make_unique<T[]>(bufferSize)){
    }
    virtual ~PaUtilRingBuffer(){}
    long GetReadAvailable()const{return ( (writeIndex.load() - readIndex.load())) & bigMask ;}
    long GetWriteAvailable()const{return bufferSize - GetReadAvailable();}
    void Flush(){
      writeIndex.store(0);
      readIndex.store(0);
    }
    long GetWriteRegions(long elementCount, T **pData0, long *pSize0,T **pData1,long *pSize1){
      auto widx = writeIndex.load();
      auto ridx = readIndex.load();
      auto available = bufferSize -((widx-ridx)&bigMask);
      if(elementCount>available){elementCount=available;}
      auto index = widx & smallMask;
      if(index+elementCount > bufferSize){
        auto firstHalf = bufferSize - index;
        *pData0 = (&buffer[index]);
        *pSize0 = firstHalf;
        *pData1 = (&buffer[0]);
        *pSize1 = elementCount-firstHalf;
      }else{
        *pData0 = (&buffer[index]);
        *pSize0 = elementCount;
        *pData1 = nullptr;
        *pSize1 = 0;
      }
      return elementCount;
    }
    long AdvanceWriteIndex(long elementCount){
      return (writeIndex.fetch_add(elementCount)+elementCount)&bigMask;
    }
    long GetReadRegions(long elementCount, T**pData0,long *pSize0,T**pData1, long *pSize1){
      auto widx = writeIndex.load();
      auto ridx = readIndex.load();
      auto available = (widx-ridx)&bigMask;
      if(elementCount>available)elementCount=available;
      auto index = ridx&smallMask;
      if(index+elementCount>bufferSize){
        long firstHalf = bufferSize-index;
        *pData0 = (&buffer[index]);
        *pSize0 = firstHalf;
        *pData1 = (&buffer[0]);
        *pSize1 = elementCount-firstHalf;
      }else{
        *pData0 = (&buffer[index]);
        *pSize0 = elementCount;
        *pData1 = nullptr;
        *pSize1 = 0;
      }
      return elementCount;
    }
    long AdvanceReadIndex(long elementCount){
      return (readIndex.fetch_add(elementCount)+elementCount)&bigMask;
    }
    long Write(const T *data, long elementCount){
      long size1, size2;
      T *data1,*data2;
      auto numWritten = GetWriteRegions(elementCount,&data1,&size1,&data2,&size2);
      if(size2>0){
        std::move(data,data+size1,data1);
        data += size1;
        std::move(data,data+size2,data2);
      }else{std::move(data,data+size1,data1);}
      AdvanceWriteIndex(numWritten);
      return numWritten;
    }
    long Read(T *data, long elementCount){
      auto size1 = long{},size2 = long{};
      T *data1,*data2;
      auto numRead = GetReadRegions(elementCount,&data1,&size1,&data2,&size2);
      if(size2>0){
        std::move(data1,data1+size1,data);
        data += size1;
        std::move(data2,data2+size1,data);
      }else{std::move(data1,data1+size1,data);}
      AdvanceReadIndex(numRead);
      return numRead;
    }
};
