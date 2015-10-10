_Pragma("once")
#include <QtDebug>
#include <QMutex>
#include <QThread>
#include <QString>
#include <atomic>

#include "cachingreaderchunk.h"
#include "trackinfoobject.h"
#include "engine/engineworker.h"
#include "sources/audiosource.h"
#include "util/fifo.h"


struct CachingReaderChunkReadRequest {
    CachingReaderChunk* chunk = nullptr;
    TrackPointer        track { };
    CachingReaderChunkReadRequest() = default;
    explicit CachingReaderChunkReadRequest(
            CachingReaderChunk* chunkArg,
            TrackPointer pTrack)
        : chunk(chunkArg),
          track(pTrack){
    }
};
enum ReaderStatus {
    INVALID,
    TRACK_NOT_LOADED,
    TRACK_LOADED,
    CHUNK_READ_SUCCESS,
    CHUNK_READ_EOF,
    CHUNK_READ_INVALID
};
struct ReaderStatusUpdate {
    ReaderStatus status = INVALID;
    CachingReaderChunk* chunk = nullptr;
    SINT maxReadableFrameIndex = 0;
    ReaderStatusUpdate() = default;
    ReaderStatusUpdate(
            ReaderStatus statusArg,
            CachingReaderChunk* chunkArg,
            SINT maxReadableFrameIndexArg)
        : status(statusArg)
        , chunk(chunkArg)
        , maxReadableFrameIndex(maxReadableFrameIndexArg) {
    }
};
class CachingReaderWorker : public EngineWorker {
    Q_OBJECT
  public:
    // Construct a CachingReader with the given group.
    CachingReaderWorker(QString group,
            FIFO<CachingReaderChunkReadRequest>* pChunkReadRequestFIFO,
            FIFO<ReaderStatusUpdate>* pReaderStatusFIFO);
    virtual ~CachingReaderWorker();
    // Request to load a new track. wake() must be called afterwards.
    virtual void newTrack(TrackPointer pTrack);
    // Run upkeep operations like loading tracks and reading from file. Run by a
    // thread pool via the EngineWorkerScheduler.
    virtual void run();
    void quitWait();
  signals:
    // Emitted once a new track is loaded and ready to be read from.
    void trackLoading();
    void trackLoaded(TrackPointer pTrack, int iSampleRate, int iNumSamples);
    void trackLoadFailed(TrackPointer pTrack, QString reason);
  private:
    QString m_group;
    QString m_tag;
    // Thread-safe FIFOs for communication between the engine callback and
    // reader thread.
    FIFO<CachingReaderChunkReadRequest>* m_pChunkReadRequestFIFO = nullptr;
    FIFO<ReaderStatusUpdate>* m_pReaderStatusFIFO = nullptr;
    // Queue of Tracks to load, and the corresponding lock. Must acquire the
    // lock to touch.
    QMutex m_newTrackMutex;
    TrackPointer m_newTrack{nullptr};
    TrackPointer m_pTrack{nullptr};
    // Internal method to load a track. Emits trackLoaded when finished.
    void loadTrack(const TrackPointer& pTrack);
    ReaderStatusUpdate processReadRequest( const CachingReaderChunkReadRequest& request);
    // The current audio source of the track loaded
    Mixxx::AudioSourcePointer m_pAudioSource;
    // The maximum readable frame index of the AudioSource. Might
    // be adjusted when decoding errors occur to prevent reading
    // the same chunk(s) over and over again.
    // This frame index references the frame that follows the
    // last frame with readable sample data.
    SINT m_maxReadableFrameIndex = -1;
    std::atomic<bool> m_stop{false};
};
