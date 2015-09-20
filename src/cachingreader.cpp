#include <QtDebug>
#include <QFileInfo>

#include "controlobject.h"
#include "controlobjectslave.h"

#include "cachingreader.h"
#include "trackinfoobject.h"
#include "sampleutil.h"
#include "util/counter.h"
#include "util/math.h"
#include "util/assert.h"

namespace {

// NOTE(uklotzde): The following comment has been adopted without
// modifications and should be rephrased.
//
// To prevent every bit of code having to guess how many samples
// forward it makes sense to keep in memory, the hinter can provide
// either 0 for a forward hint or -1 for a backward hint. We should
// be calculating an appropriate number of samples to go backward as
// some function of the latency, but for now just leave this as a
// constant. 2048 is a pretty good number of samples because 25ms
// latency corresponds to 1102.5 mono samples and we need double
// that for stereo samples.
const SINT kDefaultHintSamples = 1024 * CachingReaderChunk::kChannels;
} // anonymous namespace
// currently CachingReaderWorker::kCachingReaderChunkLength is 65536 (0x10000);
// For 80 chunks we need 5242880 (0x500000) bytes (5 MiB) of Memory
//static
const int CachingReader::maximumCachingReaderChunksInMemory = (1<<23) / (8192 * 2 * sizeof(float));
CachingReader::CachingReader(QString group, ConfigObject<ConfigValue>* config)
        : m_pConfig(config),
          m_chunkReadRequestFIFO(256),
          m_readerStatusFIFO(256),
          m_readerStatus(INVALID),
          m_mruCachingReaderChunk(nullptr),
          m_lruCachingReaderChunk(nullptr),
          m_sampleBuffer(CachingReaderChunk::kSamples * maximumCachingReaderChunksInMemory),
          m_maxReadableFrameIndex(0),
          m_worker(group, &m_chunkReadRequestFIFO, &m_readerStatusFIFO) {
    m_allocatedCachingReaderChunks.reserve(m_sampleBuffer.size());
    auto bufferStart = m_sampleBuffer.data();
    // Divide up the allocated raw memory buffer into total_chunks
    // chunks. Initialize each chunk to hold nothing and add it to the free
    // list.
    for (int i = 0; i < maximumCachingReaderChunksInMemory; ++i) {
        auto c = new CachingReaderChunkForOwner(bufferStart);
        m_chunks.push_back(c);
        m_freeChunks.push_back(c);
        bufferStart += CachingReaderChunk::kSamples;
    }
    // Forward signals from worker
    connect(&m_worker, SIGNAL(trackLoading()), this, SIGNAL(trackLoading()), Qt::DirectConnection);
    connect(&m_worker, SIGNAL(trackLoaded(TrackPointer, int, int)), this, SIGNAL(trackLoaded(TrackPointer, int, int)), Qt::DirectConnection);
    connect(&m_worker, SIGNAL(trackLoadFailed(TrackPointer, QString)), this, SIGNAL(trackLoadFailed(TrackPointer, QString)), Qt::DirectConnection);
    m_worker.start(QThread::HighPriority);
}
CachingReader::~CachingReader() {
    m_worker.quitWait();
    qDeleteAll(m_chunks);
}
void CachingReader::freeChunk(CachingReaderChunkForOwner* pChunk) {
    DEBUG_ASSERT(pChunk->getState() != CachingReaderChunkForOwner::READ_PENDING);
    auto removed = m_allocatedCachingReaderChunks.remove(pChunk->getIndex());
    // We'll tolerate not being in allocatedCachingReaderChunks,
    // because sometime you free a chunk right after you allocated it.
    DEBUG_ASSERT(removed <= 1);
    pChunk->removeFromList( &m_mruCachingReaderChunk, &m_lruCachingReaderChunk);
    pChunk->free();
    m_freeChunks.push_back(pChunk);
}
void CachingReader::freeAllChunks() {
    for (auto pChunk: m_chunks) {
        // We will receive CHUNK_READ_INVALID for all pending chunk reads
        // which should free the chunks individually.
        if (pChunk->getState() == CachingReaderChunkForOwner::READ_PENDING) 
        { 
          continue; 
        }
        if (pChunk->getState() != CachingReaderChunkForOwner::FREE) {
            pChunk->removeFromList( &m_mruCachingReaderChunk, &m_lruCachingReaderChunk);
            pChunk->free();
            m_freeChunks.push_back(pChunk);
        }
    }
    m_allocatedCachingReaderChunks.clear();
    m_mruCachingReaderChunk = nullptr;
}
CachingReaderChunkForOwner* CachingReader::allocateChunk(SINT chunkIndex) {
    if (m_freeChunks.isEmpty()) { return nullptr; }
    auto pChunk = m_freeChunks.takeFirst();
    pChunk->init(chunkIndex);
    //qDebug() << "Allocating chunk" << pChunk << pChunk->getIndex();
    m_allocatedCachingReaderChunks.insert(chunkIndex, pChunk);
    // Adjust the least-recently-used item before inserting the
    // chunk as the new most-recently-used item.
    if (!m_lruCachingReaderChunk) {
        if (!m_mruCachingReaderChunk) { m_lruCachingReaderChunk = pChunk;}
        else { m_lruCachingReaderChunk = m_mruCachingReaderChunk; }
    }
    // Insert the chunk as the new most-recently-used item.
    pChunk->insertIntoListBefore(m_mruCachingReaderChunk);
    return pChunk;
}
CachingReaderChunkForOwner* CachingReader::allocateChunkExpireLRU(SINT chunkIndex) {
    auto pChunk = allocateChunk(chunkIndex);
    if (!pChunk) {
        if (m_lruCachingReaderChunk == nullptr) {
            qDebug() << "ERROR: No LRU chunk to free in allocateChunkExpireLRU.";
            return nullptr;
        }
        freeChunk(m_lruCachingReaderChunk);
        pChunk = allocateChunk(chunkIndex);
    }
    //qDebug() << "allocateChunkExpireLRU" << chunk << pChunk;
    return pChunk;
}
CachingReaderChunkForOwner* CachingReader::lookupChunk(SINT chunkIndex) {
    // Defaults to nullptr if it's not in the hash.
    auto chunk = m_allocatedCachingReaderChunks.value(chunkIndex, nullptr);
    // Make sure the allocated number matches the indexed chunk number.
    DEBUG_ASSERT(chunk == nullptr || chunkIndex == chunk->getIndex());
    return chunk;
}
void CachingReader::freshenChunk(CachingReaderChunkForOwner* pChunk) {
    pChunk->removeFromList(&m_mruCachingReaderChunk, &m_lruCachingReaderChunk);
    pChunk->insertIntoListBefore(m_mruCachingReaderChunk);
    m_mruCachingReaderChunk = pChunk;
}
CachingReaderChunkForOwner* CachingReader::lookupChunkAndFreshen(SINT chunkIndex) {
    auto pChunk = lookupChunk(chunkIndex);
    if (pChunk ) { freshenChunk(pChunk); }
    return pChunk;
}
void CachingReader::newTrack(TrackPointer pTrack) {
    m_pTrack = pTrack;
    m_worker.newTrack(pTrack);
    m_worker.workReady();
}
void CachingReader::process() {
    auto status = ReaderStatusUpdate{};
    while (m_readerStatusFIFO.read(&status, 1) == 1) {
        auto pChunk = static_cast<CachingReaderChunkForOwner*>(status.chunk);
        if (pChunk) {
            // Take over control of the chunk from the worker.
            // This has to be done before freeing all chunks
            // after a new track has been loaded (see below)!
            pChunk->takeFromWorker();
            if (status.status != CHUNK_READ_SUCCESS) { freeChunk(pChunk); }
        }
        if (status.status == TRACK_NOT_LOADED) { m_readerStatus = status.status;}
        else if (status.status == TRACK_LOADED) {
            m_readerStatus = status.status;
            // Reset the max. readable frame index
            m_maxReadableFrameIndex = status.maxReadableFrameIndex;
            // Free all chunks with sample data from a previous track
            freeAllChunks();
        }
        // Adjust the max. readable frame index
        if (m_readerStatus == TRACK_LOADED) {
            m_maxReadableFrameIndex = math_min(status.maxReadableFrameIndex, m_maxReadableFrameIndex);
        } else { m_maxReadableFrameIndex = 0; }
    }
}
int CachingReader::read(int sample, int numSamples, CSAMPLE* buffer) {
    // Check for bad inputs
    DEBUG_ASSERT_AND_HANDLE(sample % CachingReaderChunk::kChannels == 0) {
        // This problem is easy to fix, but this type of call should be
        // complained about loudly.
        --sample;
    }
    DEBUG_ASSERT_AND_HANDLE(numSamples % CachingReaderChunk::kChannels == 0) { --numSamples; }
    if (numSamples < 0 || !buffer) {
        QString temp = QString("Sample = %1").arg(sample);
        qDebug() << "CachingReader::read() invalid arguments sample:" << sample
                 << "numSamples:" << numSamples << "buffer:" << buffer;
        return 0;
    }
    // If asked to read 0 samples, don't do anything. (this is a perfectly
    // reasonable request that happens sometimes. If no track is loaded, don't
    // do anything.
    if (numSamples == 0 || m_readerStatus != TRACK_LOADED) { return 0; }
    // Process messages from the reader thread.
    process();
    auto samplesRead = SINT{0};
    auto frameIndex = CachingReaderChunk::samples2frames(sample);
    auto numFrames = CachingReaderChunk::samples2frames(numSamples);
    // Fill the buffer up to the first readable sample with
    // silence. This may happen when the engine is in preroll,
    // i.e. if the frame index points a region before the first
    // track sample.
    if (0 > frameIndex) {
        auto prerollFrames = std::min<SINT>(numFrames, 0 - frameIndex);
        auto prerollSamples = CachingReaderChunk::frames2samples(prerollFrames);
        std::fill_n(buffer,prerollSamples,0);
        buffer += prerollSamples;
        samplesRead += prerollSamples;
        frameIndex += prerollFrames;
        numFrames -= prerollFrames;
    }
    // Read the actual samples from the audio source into the
    // buffer. The buffer will be filled with silence for every
    // unreadable sample or samples outside of the track region
    // later at the end of this function.
    if (numSamples > samplesRead) {
        // If any unread samples from the track are left the current
        // frame index must be at or beyond the first track sample.
        DEBUG_ASSERT(0 <= frameIndex);
        auto maxReadableFrameIndex = std::min<SINT>(frameIndex + numFrames, m_maxReadableFrameIndex);
        if (maxReadableFrameIndex > frameIndex) {
            // The intersection between the readable samples from the track
            // and the requested samples is not empty, so start reading.
            auto firstCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(frameIndex);
            auto lastCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(maxReadableFrameIndex - 1);
            for (auto chunkIndex = firstCachingReaderChunkIndex; chunkIndex <= lastCachingReaderChunkIndex; ++chunkIndex) {
                auto  pChunk = lookupChunkAndFreshen(chunkIndex);
                // If the chunk is not in cache, then we must return an error.
                if (!pChunk || (pChunk->getState() != CachingReaderChunkForOwner::READY)) {
                    Counter("CachingReader::read(): Failed to read chunk on cache miss")++;
                    // Exit the loop and fill the remaining buffer with silence
                    break;
                }
                // Please note that m_maxReadableFrameIndex might change with
                // every read operation! On a cache miss audio data will be
                // read from the audio source in lookupChunkAndFreshen() and
                // the max. readable frame index might be adjusted if decoding
                // errors occur.
                maxReadableFrameIndex = std::min<SINT>(maxReadableFrameIndex, m_maxReadableFrameIndex);
                if (maxReadableFrameIndex <= frameIndex) {
                    // No more readable data available. Exit the loop and
                    // fill the remaining buffer with silence.
                    break;
                }
                DEBUG_ASSERT(0 < maxReadableFrameIndex);
                lastCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(maxReadableFrameIndex - 1);
                auto  chunkFrameIndex = CachingReaderChunk::frameForIndex(chunkIndex);
                DEBUG_ASSERT(chunkFrameIndex <= frameIndex);
                DEBUG_ASSERT((chunkIndex == firstCachingReaderChunkIndex) || (chunkFrameIndex == frameIndex));
                auto chunkFrameOffset = frameIndex - chunkFrameIndex;
                DEBUG_ASSERT(chunkFrameOffset >= 0);
                auto chunkFrameCount = math_min( pChunk->getFrameCount(), maxReadableFrameIndex - chunkFrameIndex);
                if (chunkFrameCount < chunkFrameOffset) {
                    // No more readable data available from this chunk (and
                    // consequently all following chunks). Exit the loop and
                    // fill the remaining buffer with silence.
                    break;
                }
                auto framesToCopy = chunkFrameCount - chunkFrameOffset;
                DEBUG_ASSERT(framesToCopy >= 0);
                auto chunkSampleOffset = CachingReaderChunk::frames2samples(chunkFrameOffset);
                auto samplesToCopy = CachingReaderChunk::frames2samples(framesToCopy);
                pChunk->copySamples(buffer, chunkSampleOffset, samplesToCopy);
                buffer += samplesToCopy;
                samplesRead += samplesToCopy;
                frameIndex += framesToCopy;
            }
        }
    }
    // Finally fill the remaining buffer with silence.
    DEBUG_ASSERT(numSamples >= samplesRead);
    std::fill_n(buffer,numSamples-samplesRead,0);
    return numSamples;
}
void CachingReader::hintAndMaybeWake(const HintVector& hintList) {
    // If no file is loaded, skip.
    if (m_readerStatus != TRACK_LOADED) { return; }
    // For every chunk that the hints indicated, check if it is in the cache. If
    // any are not, then wake.
    auto shouldWake = false;
    for (auto hint : hintList ){
        // Handle some special length values
        if (hint.length == 0) { hint.length = kDefaultHintSamples;}
        else if (hint.length == -1) {
            hint.sample -= kDefaultHintSamples;
            hint.length = kDefaultHintSamples;
            if (hint.sample < 0) {
                hint.length += hint.sample;
                hint.sample = 0;
            }
        }
        if (hint.length < 0) {
            qDebug() << "ERROR: Negative hint length. Ignoring.";
            continue;
        }
        auto hintFrame = CachingReaderChunk::samples2frames(hint.sample);
        auto hintFrameCount = CachingReaderChunk::samples2frames(hint.length);
        auto minReadableFrameIndex = hintFrame;
        auto maxReadableFrameIndex = hintFrame + hintFrameCount;
        Mixxx::AudioSource::clampFrameInterval(minReadableFrameIndex, maxReadableFrameIndex, m_maxReadableFrameIndex);
        if (minReadableFrameIndex >= maxReadableFrameIndex) { continue; }
        const auto firstCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(minReadableFrameIndex);
        const auto lastCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(maxReadableFrameIndex - 1);
        for (auto chunkIndex = firstCachingReaderChunkIndex; chunkIndex <= lastCachingReaderChunkIndex; ++chunkIndex) {
            auto  pChunk = lookupChunk(chunkIndex);
            if (!pChunk) {
                shouldWake = true;
                pChunk = allocateChunkExpireLRU(chunkIndex);
                if (!pChunk) {
                    qDebug() << "ERROR: Couldn't allocate spare CachingReaderChunk to make CachingReaderChunkReadRequest.";
                    continue;
                }
                auto request = CachingReaderChunkReadRequest(pChunk,m_pTrack);
                pChunk->giveToWorker();
                // qDebug() << "Requesting read of chunk" << current << "into" << pChunk;
                // qDebug() << "Requesting read into " << request.chunk->data;
                if (!m_chunkReadRequestFIFO.write(&request, 1)) {
                    qWarning() << "ERROR: Could not submit read request for " << chunkIndex;
                    pChunk->takeFromWorker();
                    freeChunk(pChunk);
                }
                //qDebug() << "Checking chunk " << current << " shouldWake:" << shouldWake << " chunksToRead" << m_chunksToRead.size();
            } else if (pChunk->getState() == CachingReaderChunkForOwner::READY) {
                // This will cause the chunk to be 'freshened' in the cache. The
                // chunk will be moved to the end of the LRU list.
                freshenChunk(pChunk);
            }
        }
    }
    // If there are chunks to be read, wake up.
    if (shouldWake) { m_worker.workReady(); }
}
void CachingReader::setScheduler(EngineWorkerScheduler* pScheduler) 
{ 
  m_worker.setScheduler(pScheduler);
}
