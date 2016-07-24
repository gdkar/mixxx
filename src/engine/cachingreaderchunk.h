#ifndef ENGINE_CACHINGREADERCHUNK_H
#define ENGINE_CACHINGREADERCHUNK_H

#include "sources/audiosource.h"
#include <atomic>
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
//
// One chunk should contain 1/2 - 1/4th of a second of audio.
// 8192 frames contain about 170 ms of audio at 48 kHz, which
// is well above (hopefully) the latencies people are seeing.
// At 10 ms latency one chunk is enough for 17 callbacks.
// Additionally the chunk size should be a power of 2 for
// easier memory alignment.
// TODO(XXX): The optimum value of the "constant" kFrames depends
// on the properties of the AudioSource as the remarks above suggest!

class CachingReaderChunk {
public:
    static constexpr const int64_t kInvalidIndex = -1;
    static constexpr const int64_t kChannels = 2;
    static constexpr const int64_t kFrames = 8192;
    static constexpr const int64_t kSamples = kFrames * kChannels;

    // Returns the corresponding chunk index for a frame index
    constexpr static int64_t indexForFrame(int64_t frameIndex) { return frameIndex / kFrames; }
    // Returns the corresponding chunk index for a frame index
    constexpr static int64_t frameForIndex(int64_t chunkIndex) { return chunkIndex * kFrames;}
    // Converts frames to samples
    constexpr static int64_t frames2samples(int64_t frames) { return frames * kChannels;}
    // Converts samples to frames
    constexpr static int64_t samples2frames(int64_t samples) { return samples / kChannels;}
    // Disable copy and move constructors
    constexpr CachingReaderChunk(CSAMPLE *pBuffer)
    : m_sampleBuffer(pBuffer){}
    CachingReaderChunk(const CachingReaderChunk&) = delete;
    CachingReaderChunk(CachingReaderChunk&&) = delete;
    void init(int64_t index);
    void free();
    enum State {
        FREE,
        READY,
        READ_PENDING
    };
    State getState() const { return m_state; }
    // The state is controlled by the cache as the owner of each chunk!
    void giveToWorker() {
        DEBUG_ASSERT(READY == m_state);
        m_state = READ_PENDING;
    }
    void takeFromWorker() {
        DEBUG_ASSERT(READ_PENDING == m_state);
        m_state = READY;
    }
    // Inserts a chunk into the double-linked list before the
    // given chunk. If the list is currently empty simply pass
    // pBefore = nullptr. Please note that if pBefore points to
    // the head of the current list this chunk becomes the new
    // head of the list.
    void insertIntoListBefore(CachingReaderChunk* pBefore);
    // Removes a chunk from the double-linked list and optionally
    // adjusts head/tail pointers. Pass ppHead/ppTail = nullptr if
    // you prefer to adjust those pointers manually.
    void removeFromList(
            CachingReaderChunk** ppHead,
            CachingReaderChunk** ppTail);

    int64_t getIndex() const { return m_index; }
    bool isValid() const { return 0 <= getIndex(); }
    int64_t getFrameCount() const { return m_frameCount; }
    // Check if the audio source has sample data available
    // for this chunk.
    bool isReadable(
            const mixxx::AudioSourcePointer& pAudioSource,
            int64_t maxReadableFrameIndex) const;

    // Read sample frames from the audio source and return the
    // number of frames that have been read. The in/out parameter
    // pMaxReadableFrameIndex is adjusted if reading fails.
    int64_t readSampleFrames(
            const mixxx::AudioSourcePointer& pAudioSource,
            int64_t* pMaxReadableFrameIndex);

    // Copy sampleCount samples starting at sampleOffset from
    // the chunk's internal buffer into sampleBuffer.
    void copySamples(
            CSAMPLE* sampleBuffer,
            int64_t sampleOffset,
            int64_t sampleCount) const;

    // Copy sampleCount samples in reverse order starting at sampleOffset from
    // the chunk's internal buffer into sampleBuffer.
    void copySamplesReverse(
            CSAMPLE* sampleBuffer,
            int64_t sampleOffset,
            int64_t sampleCount) const;
private:
    int64_t              m_index{-1};
    // The worker thread will fill the sample buffer and
    // set the frame count.
    CSAMPLE* const       m_sampleBuffer;
    std::atomic<int64_t> m_frameCount{0};
    std::atomic<int64_t> m_frameOffset{0};
    std::atomic<State>   m_state{FREE};

    CachingReaderChunk* m_pPrev{nullptr}; // previous item in double-linked list
    CachingReaderChunk* m_pNext{nullptr}; // next item in double-linked list
};
#endif // ENGINE_CACHINGREADERCHUNK_H
