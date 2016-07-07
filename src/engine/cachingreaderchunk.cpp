#include "engine/cachingreaderchunk.h"

#include <QtDebug>

#include "util/math.h"
#include "util/sample.h"


CachingReaderChunk::CachingReaderChunk(
        CSAMPLE* sampleBuffer)
        : m_index(kInvalidIndex),
          m_sampleBuffer(sampleBuffer),
          m_frameCount(0)
{ }
CachingReaderChunk::~CachingReaderChunk() = default;
void CachingReaderChunk::init(int64_t index)
{
    m_index = index;
    m_frameCount = 0;
}
bool CachingReaderChunk::isReadable(
        const mixxx::AudioSourcePointer& pAudioSource,
        int64_t maxReadableFrameIndex) const {
    DEBUG_ASSERT(mixxx::AudioSource::getMinFrameIndex() <= maxReadableFrameIndex);

    if (!isValid() || pAudioSource.isNull()) {
        return false;
    }
    auto frameIndex = frameForIndex(getIndex());
    auto maxFrameIndex = math_min(maxReadableFrameIndex, pAudioSource->getMaxFrameIndex());
    return frameIndex <= maxFrameIndex;
}

int64_t CachingReaderChunk::readSampleFrames(
        const mixxx::AudioSourcePointer& pAudioSource,
        int64_t* pMaxReadableFrameIndex) {
    DEBUG_ASSERT(pMaxReadableFrameIndex);
    auto frameIndex = frameForIndex(getIndex());
    auto maxFrameIndex = math_min(*pMaxReadableFrameIndex, pAudioSource->getMaxFrameIndex());
    auto framesRemaining = *pMaxReadableFrameIndex - frameIndex;
    auto framesToRead = math_min(kFrames, framesRemaining);
    auto seekFrameIndex = pAudioSource->seekSampleFrame(frameIndex);
    if (frameIndex != seekFrameIndex) {
        // Failed to seek to the requested index. The file might
        // be corrupt and decoding should be aborted.
        qWarning() << "Failed to seek chunk position:"
                << "actual =" << seekFrameIndex
                << ", expected =" << frameIndex
                << ", maximum =" << maxFrameIndex;
        if (frameIndex >= seekFrameIndex) {
            // Simple strategy to compensate for seek inaccuracies in
            // faulty files: Try to skip some samples up to the requested
            // seek position. But only skip twice as many frames/samples
            // as have been requested to avoid decoding great portions of
            // the file for small read requests on seek errors.
            auto framesToSkip = frameIndex - seekFrameIndex;
            if (framesToSkip <= (2 * framesToRead)) {
                seekFrameIndex += pAudioSource->skipSampleFrames(framesToSkip);
            }
        }
        if (frameIndex != seekFrameIndex) {
            // Unexpected/premature end of file -> prevent further
            // seeks beyond the current seek position
            *pMaxReadableFrameIndex = math_min(seekFrameIndex, *pMaxReadableFrameIndex);
            // Don't read any samples on a seek failure!
            m_frameCount = 0;
            return m_frameCount;
        }
    }

    DEBUG_ASSERT(frameIndex == seekFrameIndex);
    DEBUG_ASSERT(CachingReaderChunk::kChannels
            == mixxx::AudioSource::kChannelCountStereo);
    m_frameCount = pAudioSource->readSampleFramesStereo(
            framesToRead, m_sampleBuffer, kSamples);
    if (m_frameCount < framesToRead) {
        qWarning() << "Failed to read chunk samples:"
                << "actual =" << m_frameCount
                << ", expected =" << framesToRead;
        // Adjust the max. readable frame index for future
        // read requests to avoid repeated invalid reads.
        *pMaxReadableFrameIndex = frameIndex + m_frameCount;
    }

    return m_frameCount;
}

void CachingReaderChunk::copySamples(
        CSAMPLE* sampleBuffer, int64_t sampleOffset, int64_t sampleCount) const {
    DEBUG_ASSERT(0 <= sampleOffset);
    DEBUG_ASSERT(0 <= sampleCount);
    DEBUG_ASSERT((sampleOffset + sampleCount) <= frames2samples(m_frameCount));
    SampleUtil::copy(sampleBuffer, m_sampleBuffer + sampleOffset, sampleCount);
}

void CachingReaderChunk::copySamplesReverse(
        CSAMPLE* sampleBuffer, int64_t sampleOffset, int64_t sampleCount) const {
    DEBUG_ASSERT(0 <= sampleOffset);
    DEBUG_ASSERT(0 <= sampleCount);
    DEBUG_ASSERT((sampleOffset + sampleCount) <= frames2samples(m_frameCount));
    SampleUtil::copyReverse(sampleBuffer, m_sampleBuffer + sampleOffset, sampleCount);
}

CachingReaderChunkForOwner::CachingReaderChunkForOwner(
        CSAMPLE* sampleBuffer)
        : CachingReaderChunk(sampleBuffer),
          m_state(FREE),
          m_pPrev(nullptr),
          m_pNext(nullptr) {
}

CachingReaderChunkForOwner::~CachingReaderChunkForOwner() {
}

void CachingReaderChunkForOwner::init(int64_t index) {
    DEBUG_ASSERT(READ_PENDING != m_state);
    CachingReaderChunk::init(index);
    m_state = READY;
}

void CachingReaderChunkForOwner::free() {
    DEBUG_ASSERT(READ_PENDING != m_state);
    CachingReaderChunk::init(kInvalidIndex);
    m_state = FREE;
}

void CachingReaderChunkForOwner::insertIntoListBefore(
        CachingReaderChunkForOwner* pBefore) {
    DEBUG_ASSERT(m_pNext == nullptr);
    DEBUG_ASSERT(m_pPrev == nullptr);
    DEBUG_ASSERT(m_state != READ_PENDING); // Must not be accessed by a worker!

    m_pNext = pBefore;
    if (pBefore) {
        if (pBefore->m_pPrev) {
            m_pPrev = pBefore->m_pPrev;
            DEBUG_ASSERT(m_pPrev->m_pNext == pBefore);
            m_pPrev->m_pNext = this;
        }
        pBefore->m_pPrev = this;
    }
}

void CachingReaderChunkForOwner::removeFromList(
        CachingReaderChunkForOwner** ppHead,
        CachingReaderChunkForOwner** ppTail) {
    // Remove this chunk from the double-linked list...
    CachingReaderChunkForOwner* pNext = m_pNext;
    CachingReaderChunkForOwner* pPrev = m_pPrev;
    m_pNext = nullptr;
    m_pPrev = nullptr;

    // ...reconnect the remaining list elements...
    if (pNext) {
        DEBUG_ASSERT(this == pNext->m_pPrev);
        pNext->m_pPrev = pPrev;
    }
    if (pPrev) {
        DEBUG_ASSERT(this == pPrev->m_pNext);
        pPrev->m_pNext = pNext;
    }

    // ...and adjust head/tail.
    if (ppHead && (this == *ppHead)) {
        *ppHead = pNext;
    }
    if (ppTail && (this == *ppTail)) {
        *ppTail = pPrev;
    }
}
