#include <QtDebug>

#include "cachingreaderchunk.h"
#include "sampleutil.h"
#include "util/math.h"

const SINT CachingReaderChunk::kInvalidIndex = -1;

// One chunk should contain 1/2 - 1/4th of a second of audio.
// 8192 frames contain about 170 ms of audio at 48 kHz, which
// is well above (hopefully) the latencies people are seeing.
// At 10 ms latency one chunk is enough for 17 callbacks.
// Additionally the chunk size should be a power of 2 for
// easier memory alignment.
// TODO(XXX): The optimum value of the "constant" kFrames depends
// on the properties of the SoundSource as the remarks above suggest!
const SINT CachingReaderChunk::kChannels = Mixxx::SoundSource::kChannelCountStereo;
const SINT CachingReaderChunk::kFrames   = 8192; // ~ 170 ms at 48 kHz
const SINT CachingReaderChunk::kSamples  = CachingReaderChunk::frames2samples(CachingReaderChunk::kFrames);
CachingReaderChunk::CachingReaderChunk( CSAMPLE* sampleBuffer)
        : m_index{kInvalidIndex},
          m_sampleBuffer{sampleBuffer},
          m_frameCount{0}
{
}
void CachingReaderChunk::init(SINT index)
{
    m_index.store(index);
    m_frameCount.store(0);
}
bool CachingReaderChunk::isReadable( const Mixxx::SoundSourcePointer& pSoundSource, SINT maxReadableFrameIndex) const
{
    DEBUG_ASSERT(0 <= maxReadableFrameIndex);
    if (!isValid() || pSoundSource.isNull()) return false;
    const SINT frameIndex = frameForIndex(getIndex());
    const SINT maxFrameIndex = math_min( maxReadableFrameIndex, pSoundSource->getMaxFrameIndex());
    return frameIndex <= maxFrameIndex;
}
SINT CachingReaderChunk::readSampleFrames(const Mixxx::SoundSourcePointer& pSoundSource, SINT* pMaxReadableFrameIndex)
{
    DEBUG_ASSERT(pMaxReadableFrameIndex);
    auto frameIndex = frameForIndex(getIndex());
    auto maxFrameIndex = math_min( *pMaxReadableFrameIndex, pSoundSource->getMaxFrameIndex());
    auto framesRemaining = *pMaxReadableFrameIndex - frameIndex;
    auto framesToRead = math_min(kFrames, framesRemaining);
    auto seekFrameIndex = pSoundSource->seekSampleFrame(frameIndex);
    if (frameIndex != seekFrameIndex)
    {
        // Failed to seek to the requested index. The file might
        // be corrupt and decoding should be aborted.
        qWarning() << "Failed to seek chunk position:"
                << "actual =" << seekFrameIndex
                << ", expected =" << frameIndex
                << ", maximum =" << maxFrameIndex;
        if (frameIndex >= seekFrameIndex)
        {
            // Simple strategy to compensate for seek inaccuracies in
            // faulty files: Try to skip some samples up to the requested
            // seek position. But only skip twice as many frames/samples
            // as have been requested to avoid decoding great portions of
            // the file for small read requests on seek errors.
            const auto framesToSkip = frameIndex - seekFrameIndex;
            if (framesToSkip <= (2 * framesToRead)) seekFrameIndex += pSoundSource->skipSampleFrames(framesToSkip);
        }
        if (frameIndex != seekFrameIndex)
        {
            // Unexpected/premature end of file -> prevent further
            // seeks beyond the current seek position
            *pMaxReadableFrameIndex = math_min(seekFrameIndex, *pMaxReadableFrameIndex);
            // Don't read any samples on a seek failure!
            m_frameCount = 0;
            return m_frameCount;
        }
    }
    DEBUG_ASSERT(frameIndex == seekFrameIndex);
    DEBUG_ASSERT(CachingReaderChunk::kChannels == Mixxx::SoundSource::kChannelCountStereo);
    m_frameCount = pSoundSource->readSampleFramesStereo(framesToRead, m_sampleBuffer, kSamples);
    if (m_frameCount < framesToRead)
    {
        qWarning() << "Failed to read chunk samples:" << " actual =" << m_frameCount << ", expected =" << framesToRead;
        // Adjust the max. readable frame index for future
        // read requests to avoid repeated invalid reads.
        *pMaxReadableFrameIndex = frameIndex + m_frameCount;
    }
    return m_frameCount;
}
void CachingReaderChunk::copySamples( CSAMPLE* sampleBuffer, SINT sampleOffset, SINT sampleCount) const
{
    DEBUG_ASSERT(0 <= sampleOffset);
    DEBUG_ASSERT(0 <= sampleCount);
    DEBUG_ASSERT((sampleOffset + sampleCount) <= frames2samples(m_frameCount));
    SampleUtil::copy(sampleBuffer, m_sampleBuffer + sampleOffset, sampleCount);
}
CachingReaderChunkForOwner::CachingReaderChunkForOwner(CSAMPLE* sampleBuffer)
        : CachingReaderChunk(sampleBuffer)
{ }
void CachingReaderChunkForOwner::init(SINT index)
{
    DEBUG_ASSERT(READ_PENDING != m_state);
    CachingReaderChunk::init(index);
    m_state.store(READY);
}
void CachingReaderChunkForOwner::free()
{
    DEBUG_ASSERT(READ_PENDING != m_state);
    CachingReaderChunk::init(kInvalidIndex);
    m_state.store(FREE);
}
void CachingReaderChunkForOwner::insertIntoListBefore(CachingReaderChunkForOwner* pBefore)
{
    DEBUG_ASSERT(!m_pNext);
    DEBUG_ASSERT(!m_pPrev);
    m_pNext = pBefore;
    if (pBefore)
    {
        if (pBefore->m_pPrev)
        {
            m_pPrev = pBefore->m_pPrev;
            DEBUG_ASSERT(m_pPrev->m_pNext == pBefore);
            m_pPrev->m_pNext = this;
        }
        pBefore->m_pPrev = this;
    }
}
void CachingReaderChunkForOwner::removeFromList(CachingReaderChunkForOwner** ppHead,CachingReaderChunkForOwner** ppTail)
{
    // Remove this chunk from the double-linked list...
    auto pNext = std::exchange(m_pNext,nullptr);
    auto pPrev = std::exchange(m_pPrev,nullptr);
    // ...reconnect the remaining list elements...
    if (pNext)
    {
        DEBUG_ASSERT(this == pNext->m_pPrev);
        pNext->m_pPrev = pPrev;
    }
    if (pPrev)
    {
        DEBUG_ASSERT(this == pPrev->m_pNext);
        pPrev->m_pNext = pNext;
    }
    // ...and adjust head/tail.
    if (ppHead && (this == *ppHead))  *ppHead = pNext;
    if (ppTail && (this == *ppTail))  *ppTail = pPrev;
}
 SINT CachingReaderChunk::indexForFrame(SINT frameIndex)
{
    DEBUG_ASSERT(0 <= frameIndex);
    return frameIndex / kFrames;
}
SINT CachingReaderChunk::frameForIndex(SINT chunkIndex)
{
    DEBUG_ASSERT(0 <= chunkIndex);
    return chunkIndex * kFrames;
}
SINT CachingReaderChunk::frames2samples(SINT frames)
{ 
  return frames * kChannels;
}
SINT CachingReaderChunk::samples2frames(SINT samples)
{
    DEBUG_ASSERT(0 == (samples % kChannels));
    return samples / kChannels;
}
SINT CachingReaderChunk::getIndex() const
{ 
  return m_index;
}
bool CachingReaderChunk::isValid() const
{ 
  return 0 <= getIndex();
}
SINT CachingReaderChunk::getFrameCount() const
{ 
  return m_frameCount; }
CachingReaderChunk::~CachingReaderChunk() = default;

CachingReaderChunkForOwner::State CachingReaderChunkForOwner::getState() const
{ 
  return m_state.load();
}
CachingReaderChunkForOwner::~CachingReaderChunkForOwner() = default;
void CachingReaderChunkForOwner::giveToWorker()
{
    DEBUG_ASSERT(READY ==  m_state.load());
    m_state.store(READ_PENDING);
}
void CachingReaderChunkForOwner::takeFromWorker()
{
    DEBUG_ASSERT(READ_PENDING == m_state.load());
    m_state.store(READY);
}
