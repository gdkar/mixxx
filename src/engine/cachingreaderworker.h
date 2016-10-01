#ifndef ENGINE_CACHINGREADERWORKER_H
#define ENGINE_CACHINGREADERWORKER_H

#include <QtDebug>
#include <QMutex>
#include <QSemaphore>
#include <QThread>
#include <QString>
#include <atomic>

#include "engine/cachingreaderchunk.h"
#include "track/track.h"
#include "engine/engineworker.h"
#include "sources/audiosource.h"
#include "util/fifo.h"



struct ReaderStatusUpdate {
    Q_GADGET
    Q_PROPERTY(ReaderStatus status MEMBER status)
    Q_PROPERTY(CachingReaderChunk* chunk MEMBER chunk)
    Q_PROPERTY(qint64 maxReadableFrameIndex MEMBER maxReadableFrameIndex)
public:
    enum ReaderStatus {
        INVALID,
        TRACK_NOT_LOADED,
        TRACK_LOADED,
        CHUNK_READ_SUCCESS,
        CHUNK_READ_EOF,
        CHUNK_READ_INVALID
    };
    Q_ENUM(ReaderStatus)

    ReaderStatus status{INVALID};
    CachingReaderChunk* chunk{};
    SINT maxReadableFrameIndex{};
    constexpr ReaderStatusUpdate() = default;

    constexpr ReaderStatusUpdate(
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
            intrusive_fifo<CachingReaderChunk>& pFreeChunks,
            intrusive_fifo<CachingReaderChunk>& pChunkReadRequestFIFO,
            FIFO<ReaderStatusUpdate>& pReaderStatusFIFO,
            QObject *pParent);
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
    intrusive_fifo<CachingReaderChunk>& m_freeChunks;;
    intrusive_fifo<CachingReaderChunk>& m_pChunkReadRequestFIFO;
    fifo<ReaderStatusUpdate>& m_pReaderStatusFIFO;

    // Queue of Tracks to load, and the corresponding lock. Must acquire the
    // lock to touch.
    QMutex m_newTrackMutex;
    TrackPointer m_newTrack;
    // Internal method to load a track. Emits trackLoaded when finished.
    void loadTrack(const TrackPointer& pTrack);

    ReaderStatusUpdate processReadRequest(CachingReaderChunk* request);
    // The current audio source of the track loaded
    mixxx::AudioSourcePointer m_pAudioSource;
    // The maximum readable frame index of the AudioSource. Might
    // be adjusted when decoding errors occur to prevent reading
    // the same chunk(s) over and over again.
    // This frame index references the frame that follows the
    // last frame with readable sample data.
    SINT m_maxReadableFrameIndex;
    std::atomic<bool> m_stop;
};


#endif /* ENGINE_CACHINGREADERWORKER_H */
