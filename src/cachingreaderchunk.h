_Pragma("once")
#include "sources/soundsource.h"
#include <atomic>
#include <utility>
#include <algorithm>

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
class CachingReaderChunk {
public:
    static const SINT kInvalidIndex;
    static const SINT kChannels;
    static const SINT kFrames;
    static const SINT kSamples;
    // Returns the corresponding chunk index for a frame index
    static SINT indexForFrame(SINT frameIndex);
    // Returns the corresponding chunk index for a frame index
    static SINT frameForIndex(SINT chunkIndex);
    // Converts frames to samples
    static SINT frames2samples(SINT frames);
    // Converts samples to frames
    static SINT samples2frames(SINT samples);
    // Disable copy and move constructors
    CachingReaderChunk(const CachingReaderChunk&) = delete;
    CachingReaderChunk(CachingReaderChunk&&) = delete;
    virtual SINT getIndex() const;
    virtual bool isValid() const;
    virtual SINT getFrameCount() const;
    // Check if the audio source has sample data available
    // for this chunk.
    virtual bool isReadable( Mixxx::SoundSourcePointer pSoundSource, SINT maxReadableFrameIndex) const;
    // Read sample frames from the audio source and return the
    // number of frames that have been read. The in/out parameter
    // pMaxReadableFrameIndex is adjusted if reading fails.
    virtual SINT readSampleFrames( Mixxx::SoundSourcePointer pSoundSource, SINT* pMaxReadableFrameIndex);
    // Copy sampleCount samples starting at sampleOffset from
    // the chunk's internal buffer into sampleBuffer.
    void copySamples( CSAMPLE* sampleBuffer, SINT sampleOffset, SINT sampleCount) const;
protected:
    explicit CachingReaderChunk(CSAMPLE* sampleBuffer);
    virtual ~CachingReaderChunk();
    virtual void init(SINT index);
private:
    std::atomic<SINT> m_index { 0 };
    // The worker thread will fill the sample buffer and
    // set the frame count.
    CSAMPLE* const m_sampleBuffer = nullptr;
    std::atomic<SINT> m_frameCount { 0 };
};
// This derived class is only accessible for the cache as the owner,
// but not the worker thread. The state READ_PENDING indicates that
// the worker thread is in control.
class CachingReaderChunkForOwner: public CachingReaderChunk {
public:
    explicit CachingReaderChunkForOwner(CSAMPLE* sampleBuffer);
    virtual ~CachingReaderChunkForOwner();
    virtual void init(SINT index);
    virtual void free();
    enum State {
        FREE,
        READY,
        READ_PENDING
    };
    virtual State getState() const;
    // The state is controlled by the cache as the owner of each chunk!
    virtual void giveToWorker();
    virtual void takeFromWorker();
    // Inserts a chunk into the double-linked list before the
    // given chunk. If the list is currently empty simply pass
    // pBefore = nullptr. Please note that if pBefore points to
    // the head of the current list this chunk becomes the new
    // head of the list.
    void insertIntoListBefore( CachingReaderChunkForOwner* pBefore);
    // Removes a chunk from the double-linked list and optionally
    // adjusts head/tail pointers. Pass ppHead/ppTail = nullptr if
    // you prefer to adjust those pointers manually.
    void removeFromList( CachingReaderChunkForOwner** ppHead, CachingReaderChunkForOwner** ppTail);
private:
    std::atomic<State> m_state{FREE};
    CachingReaderChunkForOwner* m_pPrev = nullptr; // previous item in double-linked list
    CachingReaderChunkForOwner* m_pNext = nullptr; // next item in double-linked list
};
