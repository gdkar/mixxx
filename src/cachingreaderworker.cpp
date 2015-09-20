#include <QtDebug>
#include <QFileInfo>
#include <QMutexLocker>

#include "controlobject.h"
#include "controlobjectslave.h"

#include "cachingreaderworker.h"
#include "soundsourceproxy.h"
#include "util/compatibility.h"
#include "util/event.h"


CachingReaderWorker::CachingReaderWorker(
        QString group,
        FIFO<CachingReaderChunkReadRequest>* pChunkReadRequestFIFO,
        FIFO<ReaderStatusUpdate>* pReaderStatusFIFO)
        : m_group(group),
          m_tag(QString("CachingReaderWorker %1").arg(m_group)),
          m_pChunkReadRequestFIFO(pChunkReadRequestFIFO),
          m_pReaderStatusFIFO(pReaderStatusFIFO),
          m_maxReadableFrameIndex(0),
          m_stop{false} {}
CachingReaderWorker::~CachingReaderWorker() = default;
ReaderStatusUpdate CachingReaderWorker::processReadRequest(const CachingReaderChunkReadRequest& request) {
    auto pChunk = request.chunk;
    DEBUG_ASSERT(pChunk);
    if ( request.track != m_pTrack )
    {
      return ReaderStatusUpdate(CHUNK_READ_INVALID,pChunk,m_maxReadableFrameIndex);
    }
    ReaderStatus status;
    // Before trying to read any data we need to check if the audio source
    // is available and if any audio data that is needed by the chunk is
    // actually available.
    if (!pChunk->isReadable(m_pAudioSource, m_maxReadableFrameIndex)) 
    {
        return ReaderStatusUpdate(CHUNK_READ_INVALID, pChunk, m_maxReadableFrameIndex);
    }
    // Try to read the data required for the chunk from the audio source
    // and adjust the max. readable frame index if decoding errors occur.
    const SINT framesRead = pChunk->readSampleFrames( m_pAudioSource, &m_maxReadableFrameIndex);
    if (0 < framesRead) { status = CHUNK_READ_SUCCESS;}
    else {
        // If no data has been read we need to distinguish two different
        // cases. If the chunk claims that the audio source is still readable
        // even with the resulting (and possibly modified) max. frame index
        // we have simply reached the end of the track. Otherwise the chunk
        // is beyond the readable range of the audio source and we need to
        // declare this read request as invalid.
        if (pChunk->isReadable(m_pAudioSource, m_maxReadableFrameIndex)) { status = CHUNK_READ_EOF;}
        else { status = CHUNK_READ_INVALID; }
    }
    return ReaderStatusUpdate(status, pChunk, m_maxReadableFrameIndex);
}
// WARNING: Always called from a different thread (GUI)
void CachingReaderWorker::newTrack(TrackPointer pTrack) {
    QMutexLocker locker(&m_newTrackMutex);
    m_newTrack = pTrack;
}
void CachingReaderWorker::run() {
    unsigned static id = 0; //the id of this thread, for debugging purposes
    QThread::currentThread()->setObjectName(QString("CachingReaderWorker %1").arg(++id));
    auto request = CachingReaderChunkReadRequest{};
    Event::start(m_tag);
    while (!(m_stop.load())) {
        if (m_newTrack) {
            auto pLoadTrack = TrackPointer{nullptr};
            m_newTrack.swap(pLoadTrack);
            loadTrack(pLoadTrack);
        } else if (m_pChunkReadRequestFIFO->read(&request, 1) == 1) {
            // Read the requested chunk and send the result
            auto update =  processReadRequest(request);
            m_pReaderStatusFIFO->writeBlocking(&update, 1);
        } else {
            Event::end(m_tag);
            m_semaRun.acquire();
            Event::start(m_tag);
        }
    }
}
namespace
{
    Mixxx::AudioSourcePointer openAudioSourceForReading(const TrackPointer& pTrack, const Mixxx::AudioSourceConfig& audioSrcCfg) {
        SoundSourceProxy soundSourceProxy(pTrack);
        auto  pAudioSource = soundSourceProxy.openAudioSource(audioSrcCfg);
        if (pAudioSource.isNull()) {
            qWarning() << "Failed to open file:" << pTrack->getLocation();
            return Mixxx::AudioSourcePointer();
        }
        // successfully opened and readable
        return pAudioSource;
    }
}
void CachingReaderWorker::loadTrack(const TrackPointer& pTrack) {
    //qDebug() << m_group << "CachingReaderWorker::loadTrack() lock acquired for load.";
    // Emit that a new track is loading, stops the current track
    emit(trackLoading());
    auto status = ReaderStatusUpdate{};
    status.status = TRACK_NOT_LOADED;
    auto filename = pTrack->getLocation();
    if (filename.isEmpty() || !pTrack->exists()) {
        // Must unlock before emitting to avoid deadlock
        qDebug() << m_group << "CachingReaderWorker::loadTrack() load failed for\"" << filename << "\", unlocked reader lock";
        m_pReaderStatusFIFO->writeBlocking(&status, 1);
        emit(trackLoadFailed(pTrack, QString("The file '%1' could not be found.").arg(filename)));
        return;
    }
    auto audioSrcCfg = Mixxx::AudioSourceConfig { CachingReaderChunk::kChannels,-1};
    m_pAudioSource = openAudioSourceForReading(pTrack, audioSrcCfg);
    if (m_pAudioSource.isNull()) {
        m_maxReadableFrameIndex = 0;
        // Must unlock before emitting to avoid deadlock
        qDebug() << m_group << "CachingReaderWorker::loadTrack() load failed for\"" << filename << "\", file invalid, unlocked reader lock";
        m_pReaderStatusFIFO->writeBlocking(&status, 1);
        emit(trackLoadFailed( pTrack, QString("The file '%1' could not be loaded.").arg(filename)));
        return;
    }
    // Initially assume that the complete content offered by audio source
    // is available for reading. Later if read errors occur this value will
    // be decreased to avoid repeated reading of corrupt audio data.
    m_maxReadableFrameIndex = m_pAudioSource->getMaxFrameIndex();
    status.maxReadableFrameIndex = m_maxReadableFrameIndex;
    status.status = TRACK_LOADED;
    m_pReaderStatusFIFO->writeBlocking(&status, 1);
    // Clear the chunks to read list.
    auto request = CachingReaderChunkReadRequest{};
    while (m_pChunkReadRequestFIFO->read(&request, 1) == 1 && request.track != pTrack)
    {
        qDebug() << "Skipping read request for " << request.chunk->getIndex();
        status.status = CHUNK_READ_INVALID;
        status.chunk = request.chunk;
        m_pReaderStatusFIFO->writeBlocking(&status, 1);
    }
    // Emit that the track is loaded.
    auto sampleCount = CachingReaderChunk::frames2samples( m_pAudioSource->getFrameCount());
    m_pTrack = pTrack;
    emit(trackLoaded(pTrack, m_pAudioSource->getFrameRate(), sampleCount));
}
void CachingReaderWorker::quitWait() {
    m_stop.store(true);
    m_semaRun.release();
    wait();
}
