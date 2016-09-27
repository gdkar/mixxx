#ifndef ENGINE_CACHINGREADERCHUNK_H
#define ENGINE_CACHINGREADERCHUNK_H

#include "sources/audiosource.h"
#include "util/intrusive_fifo.hpp"
#include <utility>
#include <memory>
// A Chunk is a memory-resident section of audio that has been cached.
// Each chunk holds a fixed number kFrames of frames with samples for
// kChannels.
//
// The class is not thread-safe although it is shared between CachingReader
// and CachingReaderWorker! A lock-free FIFO ensures that only a single
// thread has exclusive access on each chunk. This abstract base class
// is available for both the worker thread and the cache.
//
// This is the common (abstract) base class for both the cache (as the owner)
// and the worker.
class CachingReaderChunk : public intrusive_node {
public:
    static constexpr SINT kInvalidIndex = -1;
    static constexpr SINT kChannels     =  mixxx::AudioSignal::kChannelCountStereo;
    static constexpr SINT kFrames       = 8192;
    static constexpr SINT kSamples = kFrames * kChannels;
    // Converts frames to samples
    static constexpr SINT frames2samples(SINT frames)
    {
        return frames * kChannels;
    }
    // Converts samples to frames
    static constexpr SINT samples2frames(SINT samples)
    {
        return samples / kChannels;
    }
    // Returns the corresponding chunk index for a frame index
    static constexpr SINT indexForFrame(SINT frameIndex)
    {
        return frameIndex / kFrames;
    }
    // Returns the corresponding chunk index for a frame index
    static constexpr SINT frameForIndex(SINT chunkIndex)
    {
        return chunkIndex * kFrames;
    }

    // Disable copy and move constructors
    CachingReaderChunk(const CachingReaderChunk&) = delete;
    CachingReaderChunk(CachingReaderChunk&&) = delete;

    SINT getIndex() const;
    bool isValid() const;
    SINT getFrameCount() const;
    // Check if the audio source has sample data available
    // for this chunk.
    bool isReadable(
            const mixxx::AudioSourcePointer& pAudioSource,
            SINT maxReadableFrameIndex) const;

    // Read sample frames from the audio source and return the
    // number of frames that have been read. The in/out parameter
    // pMaxReadableFrameIndex is adjusted if reading fails.
    SINT readSampleFrames(
            const mixxx::AudioSourcePointer& pAudioSource,
            SINT* pMaxReadableFrameIndex);

    // Copy sampleCount samples starting at sampleOffset from
    // the chunk's internal buffer into sampleBuffer.
    void copySamples(
            CSAMPLE* sampleBuffer,
            SINT sampleOffset,
            SINT sampleCount) const;

    // Copy sampleCount samples in reverse order starting at sampleOffset from
    // the chunk's internal buffer into sampleBuffer.
    void copySamplesReverse(
            CSAMPLE* sampleBuffer,
            SINT sampleOffset,
            SINT sampleCount) const;

    enum State {
        FREE,
        READY,
        READ_PENDING
    };

    State getState() const;
    // The state is controlled by the cache as the owner of each chunk!
    void giveToWorker();
    void takeFromWorker();
    // Inserts a chunk into the double-linked list before the
    // given chunk. If the list is currently empty simply pass
    // pBefore = nullptr. Please note that if pBefore points to
    // the head of the current list this chunk becomes the new
    // head of the list.
    void insertIntoListBefore(
            CachingReaderChunk* pBefore);
    // Removes a chunk from the double-linked list and optionally
    // adjusts head/tail pointers. Pass ppHead/ppTail = nullptr if
    // you prefer to adjust those pointers manually.
    void removeFromList(
            CachingReaderChunk** ppHead,
            CachingReaderChunk** ppTail);

    void init(SINT index);
    void free();
    explicit CachingReaderChunk();
    virtual ~CachingReaderChunk();
protected:
    std::atomic<SINT> m_index{};
    std::atomic<SINT> m_frameCount{};
    std::unique_ptr<CSAMPLE[]> m_sampleBuffer{};
    State m_state;

    CachingReaderChunk* m_pPrev{}; // previous item in double-linked list
    CachingReaderChunk* m_pNext{}; // next item in double-linked list

    // The worker thread will fill the sample buffer and
    // set the frame count.
};

#endif // ENGINE_CACHINGREADERCHUNK_H
