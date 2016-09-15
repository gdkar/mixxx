#include <QtDebug>
#include <QFileInfo>
#include "control/controlobject.h"

#include "engine/cachingreaderworker.h"
#include "sources/soundsourceproxy.h"
#include "util/event.h"


CachingReaderWorker::CachingReaderWorker(
        QString group,
        FIFO<CachingReaderChunk*>* pChunkReadRequestFIFO,
        FIFO<ReaderStatusUpdate>* pReaderStatusFIFO)
        : m_group(group),
          m_tag(QString("CachingReaderWorker %1").arg(m_group)),
          m_pChunkReadRequestFIFO(pChunkReadRequestFIFO),
          m_pReaderStatusFIFO(pReaderStatusFIFO),
          m_maxReadableFrameIndex(mixxx::AudioSource::getMinFrameIndex()),
          m_stop(0)
{ }

CachingReaderWorker::~CachingReaderWorker() {
}

ReaderStatusUpdate CachingReaderWorker::processReadRequest(CachingReaderChunk* pChunk)
{
    DEBUG_ASSERT(pChunk);

    // Before trying to read any data we need to check if the audio source
    // is available and if any audio data that is needed by the chunk is
    // actually available.
    if (!pChunk->isReadable(m_pAudioSource, m_maxReadableFrameIndex)) {
        return ReaderStatusUpdate(CHUNK_READ_INVALID, pChunk, m_maxReadableFrameIndex);
    }

    // Try to read the data required for the chunk from the audio source
    // and adjust the max. readable frame index if decoding errors occur.
    auto framesRead = pChunk->readSampleFrames(m_pAudioSource, &m_maxReadableFrameIndex);

    ReaderStatus status;
    if (0 < framesRead) {
        status = CHUNK_READ_SUCCESS;
    } else {
        // If no data has been read we need to distinguish two different
        // cases. If the chunk claims that the audio source is still readable
        // even with the resulting (and possibly modified) max. frame index
        // we have simply reached the end of the track. Otherwise the chunk
        // is beyond the readable range of the audio source and we need to
        // declare this read request as invalid.
        if (pChunk->isReadable(m_pAudioSource, m_maxReadableFrameIndex)) {
            status = CHUNK_READ_EOF;
        } else {
            status = CHUNK_READ_INVALID;
        }
    }
    return ReaderStatusUpdate(status, pChunk, m_maxReadableFrameIndex);
}

// WARNING: Always called from a different thread (GUI)
void CachingReaderWorker::newTrack(TrackPointer pTrack)
{
    m_newTrack.swap(pTrack);
}

void CachingReaderWorker::run()
{
    unsigned static id = 0; //the id of this thread, for debugging purposes
    QThread::currentThread()->setObjectName(QString("CachingReaderWorker %1").arg(++id));

    Event::start(m_tag);
    while (!m_stop.load()) {
        if (m_newTrack) {
            auto pLoadTrack = TrackPointer{};
            pLoadTrack.swap(m_newTrack);
            if(pLoadTrack){
                loadTrack(pLoadTrack);
            }
        } else if (!m_pChunkReadRequestFIFO->empty()){
            auto request = m_pChunkReadRequestFIFO->front();
            m_pChunkReadRequestFIFO->pop_front();
            // Read the requested chunk and send the result
            auto update = processReadRequest(request);
            while(m_pReaderStatusFIFO->full()){}
            m_pReaderStatusFIFO->push_back(update);
        } else {
            Event::end(m_tag);
            m_semaRun.acquire();
            Event::start(m_tag);
        }
    }
}
namespace
{
    mixxx::AudioSourcePointer openAudioSourceForReading(TrackPointer pTrack, const mixxx::AudioSourceConfig& audioSrcCfg)
    {
        SoundSourceProxy soundSourceProxy(pTrack);
        auto pAudioSource = soundSourceProxy.openAudioSource(audioSrcCfg);
        if (!pAudioSource) {
            qWarning() << "Failed to open file:" << pTrack->getLocation();
            return mixxx::AudioSourcePointer();
        }
        // successfully opened and readable
        return pAudioSource;
    }
}
void CachingReaderWorker::loadTrack(const TrackPointer& pTrack)
{
    //qDebug() << m_group << "CachingReaderWorker::loadTrack() lock acquired for load.";

    // Emit that a new track is loading, stops the current track
    emit(trackLoading());

    ReaderStatusUpdate status;
    status.status = TRACK_NOT_LOADED;

    auto filename = pTrack->getLocation();
    if (filename.isEmpty() || !pTrack->exists()) {
        // Must unlock before emitting to avoid deadlock
        qDebug() << m_group << "CachingReaderWorker::loadTrack() load failed for\""
                 << filename << "\", unlocked reader lock";
        m_pReaderStatusFIFO->writeBlocking(&status, 1);
        emit(trackLoadFailed(pTrack, QString("The file '%1' could not be found.").arg(filename)));
        return;
    }

    mixxx::AudioSourceConfig audioSrcCfg;
    audioSrcCfg.setChannelCount(CachingReaderChunk::kChannels);
    m_pAudioSource = openAudioSourceForReading(pTrack, audioSrcCfg);
    if (!m_pAudioSource) {
        m_maxReadableFrameIndex = mixxx::AudioSource::getMinFrameIndex();
        // Must unlock before emitting to avoid deadlock
        qDebug() << m_group << "CachingReaderWorker::loadTrack() load failed for\""
                 << filename << "\", file invalid, unlocked reader lock";

        while(m_pReaderStatusFIFO->full()){}
        m_pReaderStatusFIFO->push_back(status);
        emit(trackLoadFailed(pTrack, QString("The file '%1' could not be loaded.").arg(filename)));
        return;
    }

    // Initially assume that the complete content offered by audio source
    // is available for reading. Later if read errors occur this value will
    // be decreased to avoid repeated reading of corrupt audio data.
    m_maxReadableFrameIndex = m_pAudioSource->getMaxFrameIndex();

    status.maxReadableFrameIndex = m_maxReadableFrameIndex;
    status.status = TRACK_LOADED;
    while(m_pReaderStatusFIFO->full()){}
    m_pReaderStatusFIFO->push_back(status);

    // Clear the chunks to read list.
    while(!m_pChunkReadRequestFIFO->empty()){
        auto request = m_pChunkReadRequestFIFO->front();
        m_pChunkReadRequestFIFO->pop_front();
        qDebug() << "Skipping read request for " << request->getIndex();
        status.status = CHUNK_READ_INVALID;
        status.chunk = request;
        while(m_pReaderStatusFIFO->full()){}
        m_pReaderStatusFIFO->push_back(status);
    }

    // Emit that the track is loaded.
    auto sampleCount =CachingReaderChunk::frames2samples(m_pAudioSource->getFrameCount());
    emit(trackLoaded(pTrack, m_pAudioSource->getSampleRate(), sampleCount));
}
void CachingReaderWorker::quitWait()
{
    m_stop = 1;
    m_semaRun.release();
    wait();
}
