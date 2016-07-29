/*
 * $Id: pa_ringbuffer.c 1738 2011-08-18 11:47:28Z rossb $
 * Portable Audio I/O Library
 * Ring Buffer utility.
 *
 * Author: Phil Burk, http://www.softsynth.com
 * modified for SMP safety on Mac OS X by Bjorn Roche
 * modified for SMP safety on Linux by Leland Lucius
 * also, allowed for const where possible
 * modified for multiple-byte-sized data elements by Sven Fischer
 *
 * Note that this is safe only for a single-thread reader and a
 * single-thread writer.
 *
 * This program uses the PortAudio Portable Audio Library.
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

/**
 @file
 @ingroup common_src
*/

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <utility>

#include "util/pa_ringbuffer.h"

/***************************************************************************
 * Initialize FIFO.
 * elementCount must be power of 2, returns -1 if not.
 */
size_t PaUtil_InitializeRingBuffer( PaUtilRingBuffer *rbuf, size_t itemSize, size_t elementCount, void *dataPtr )
{
    if((!elementCount || ((elementCount-1) & elementCount)))
        return -1; /* Not Power of two. */
    rbuf->bufferSize = elementCount;
    rbuf->buffer = (char *)dataPtr;
    PaUtil_FlushRingBuffer( rbuf );
    rbuf->bigMask = (elementCount*2)-1;
    rbuf->bufferMask = (elementCount)-1;
    rbuf->itemSize = itemSize;
    return 0;
}

/***************************************************************************
** Return number of elements available for reading. */
size_t PaUtil_GetRingBufferReadAvailable( const PaUtilRingBuffer *rbuf )
{
    return ( (rbuf->writeIndex.load() - rbuf->readIndex.load()) & rbuf->bigMask );
}
/***************************************************************************
** Return number of elements available for writing. */
size_t PaUtil_GetRingBufferWriteAvailable( const PaUtilRingBuffer *rbuf )
{
    return ( rbuf->bufferSize - PaUtil_GetRingBufferReadAvailable(rbuf));
}
/***************************************************************************
** Clear buffer. Should only be called when buffer is NOT being read or written. */
void PaUtil_FlushRingBuffer( PaUtilRingBuffer *rbuf )
{
    rbuf->readIndex.store(rbuf->writeIndex.load());
}
/***************************************************************************
** Get address of region(s) to which we can write data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be written or elementCount, whichever is smaller.
*/
size_t PaUtil_GetRingBufferWriteRegions( PaUtilRingBuffer *rbuf, size_t elementCount,
                                       void **dataPtr1, size_t *sizePtr1,
                                       void **dataPtr2, size_t *sizePtr2 )
{
    auto available = PaUtil_GetRingBufferWriteAvailable( rbuf );
    if( elementCount > available )
        elementCount = available;
    /* Check to see if write is not contiguous. */
    auto index = rbuf->writeIndex.load() & rbuf->bufferMask;
    if( (index + elementCount) > rbuf->bufferSize )
    {
        /* Write data in two blocks that wrap the buffer. */
        auto firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index*rbuf->itemSize];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = elementCount - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index*rbuf->itemSize];
        *sizePtr1 = elementCount;
        *dataPtr2 = nullptr;
        *sizePtr2 = 0;
    }
    return elementCount;
}
/***************************************************************************
*/
size_t PaUtil_AdvanceRingBufferWriteIndex( PaUtilRingBuffer *rbuf, size_t elementCount )
{
    /* ensure that previous writes are seen before we update the write index
       (write after write)
    */
    auto writeIndex = (rbuf->writeIndex.load() + elementCount) & rbuf->bigMask;
    rbuf->writeIndex.store(writeIndex);
    return writeIndex;

}

/***************************************************************************
** Get address of region(s) from which we can read data.
** If the region is contiguous, size2 will be zero.
** If non-contiguous, size2 will be the size of second region.
** Returns room available to be read or elementCount, whichever is smaller.
*/
size_t PaUtil_GetRingBufferReadRegions( PaUtilRingBuffer *rbuf, size_t elementCount,
                                void **dataPtr1, size_t *sizePtr1,
                                void **dataPtr2, size_t *sizePtr2 )
{
    auto available = PaUtil_GetRingBufferReadAvailable( rbuf ); /* doesn't use memory barrier */
    if( elementCount > available )
        elementCount = available;
    /* Check to see if read is not contiguous. */
    auto index = rbuf->readIndex.load() & rbuf->bufferMask;
    if( (index + elementCount) > rbuf->bufferSize )
    {
        /* Write data in two blocks that wrap the buffer. */
        auto firstHalf = rbuf->bufferSize - index;
        *dataPtr1 = &rbuf->buffer[index*rbuf->itemSize];
        *sizePtr1 = firstHalf;
        *dataPtr2 = &rbuf->buffer[0];
        *sizePtr2 = elementCount - firstHalf;
    }
    else
    {
        *dataPtr1 = &rbuf->buffer[index*rbuf->itemSize];
        *sizePtr1 = elementCount;
        *dataPtr2 = nullptr;
        *sizePtr2 = 0;
    }
    return elementCount;
}
/***************************************************************************
*/
size_t PaUtil_AdvanceRingBufferReadIndex( PaUtilRingBuffer *rbuf, size_t elementCount )
{
    /* ensure that previous reads (copies out of the ring buffer) are always completed before updating (writing) the read index.
       (write-after-read) => full barrier
    */
    auto readIndex = (rbuf->readIndex.load() + elementCount) & rbuf->bigMask;
    rbuf->readIndex.store(readIndex);
    return readIndex;
}
/***************************************************************************
** Return elements written. */
size_t PaUtil_WriteRingBuffer( PaUtilRingBuffer *rbuf, const void *data, size_t elementCount )
{
    size_t size1, size2, numWritten;
    void *data1, *data2;
    numWritten = PaUtil_GetRingBufferWriteRegions( rbuf, elementCount, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {

        std::memmove( data1, data, size1*rbuf->itemSize );
        data = ((char *)data) + size1*rbuf->itemSize;
        std::memmove( data2, data, size2*rbuf->itemSize );
    }
    else
    {
        std::memmove( data1, data, size1*rbuf->itemSize );
    }
    PaUtil_AdvanceRingBufferWriteIndex( rbuf, numWritten );
    return numWritten;
}

/***************************************************************************
** Return elements read. */
size_t PaUtil_ReadRingBuffer( PaUtilRingBuffer *rbuf, void *data, size_t elementCount )
{
    size_t size1, size2, numRead;
    void *data1, *data2;
    numRead = PaUtil_GetRingBufferReadRegions( rbuf, elementCount, &data1, &size1, &data2, &size2 );
    if( size2 > 0 )
    {
        std::memmove( data, data1, size1*rbuf->itemSize );
        data = ((char *)data) + size1*rbuf->itemSize;
        std::memmove( data, data2, size2*rbuf->itemSize );
    }
    else
    {
        std::memmove( data, data1, size1*rbuf->itemSize );
    }
    PaUtil_AdvanceRingBufferReadIndex( rbuf, numRead );
    return numRead;
}
