#include <QtDebug>
#include <QFileInfo>

#include "engine/cachingreader.h"
#include "control/controlobject.h"
#include "track/track.h"
#include "util/assert.h"
#include "util/counter.h"
#include "util/math.h"
#include "util/sample.h"

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
const int CachingReader::maximumCachingReaderChunksInMemory = 256;

CachingReader::CachingReader(QString group,
                             UserSettingsPointer config)
        : m_pConfig(config),
          m_readerStatusFIFO(1024),
          m_readerStatus(ReaderStatusUpdate::INVALID),
          m_mruCachingReaderChunk(nullptr),
          m_lruCachingReaderChunk(nullptr),
          m_maxReadableFrameIndex(mixxx::AudioSource::getMinFrameIndex()),
          m_worker(new CachingReaderWorker(
            group,
            m_freeChunks,
            m_chunkReadRequestFIFO,
            m_readerStatusFIFO,
            this))
{
    m_allocatedCachingReaderChunks.reserve(maximumCachingReaderChunksInMemory);
    // Divide up the allocated raw memory buffer into total_chunks
    // chunks. Initialize each chunk to hold nothing and add it to the free
    // list.
    for (auto i = 0; i < maximumCachingReaderChunksInMemory; ++i) {
        m_chunks.emplace_back(std::make_unique<CachingReaderChunk>());
        m_freeChunks.push(m_chunks.back().get());
    }
    // Forward signals from worker
    connect(m_worker, &CachingReaderWorker::trackLoading,
            this, &CachingReader::trackLoading,Qt::AutoConnection);
    connect(m_worker, &CachingReaderWorker::trackLoaded,
            this, &CachingReader::trackLoaded,Qt::AutoConnection);
    connect(m_worker, &CachingReaderWorker::trackLoadFailed,
            this, &CachingReader::trackLoadFailed,Qt::AutoConnection);

    m_worker->start(QThread::HighPriority);
}
CachingReader::~CachingReader()
{
    m_worker->quitWait();
    while(m_chunkReadRequestFIFO.take()) { /* nothing */ }
    while(m_freeChunks.take()) { /* nothing */ }
    m_chunks.clear();
}
void CachingReader::freeChunk(CachingReaderChunk* pChunk)
{
    DEBUG_ASSERT(pChunk != nullptr);
    DEBUG_ASSERT(pChunk->getState() != CachingReaderChunk::PENDING);
    auto removed = m_allocatedCachingReaderChunks.remove(pChunk->getIndex());
    // We'll tolerate not being in allocatedCachingReaderChunks,
    // because sometime you free a chunk right after you allocated it.
    DEBUG_ASSERT(removed <= 1);
    pChunk->removeFromList(&m_mruCachingReaderChunk, &m_lruCachingReaderChunk);
    pChunk->free();
    m_freeChunks.push(pChunk);
}
void CachingReader::freeAllChunks()
{
    for (auto & pChunk: m_chunks) {
        // We will receive CHUNK_READ_INVALID for all pending chunk reads
        // which should free the chunks individually.
        if (pChunk->getState() == CachingReaderChunk::PENDING) {
            continue;
        }
        if (pChunk->getState() != CachingReaderChunk::FREE) {
            pChunk->removeFromList(&m_mruCachingReaderChunk, &m_lruCachingReaderChunk);
            pChunk->free();
            m_freeChunks.push(pChunk.get());
        }
    }
    m_allocatedCachingReaderChunks.clear();
    m_mruCachingReaderChunk = nullptr;
}
CachingReaderChunk* CachingReader::allocateChunk(SINT chunkIndex)
{
    if (!m_freeChunks.empty()) {
        if(auto pChunk = m_freeChunks.take()){
            pChunk->init(chunkIndex);
            //qDebug() << "Allocating chunk" << pChunk << pChunk->getIndex();
            m_allocatedCachingReaderChunks.insert(chunkIndex, pChunk);
            return pChunk;
        }
    }
    return nullptr;
}
CachingReaderChunk* CachingReader::allocateChunkExpireLRU(SINT chunkIndex)
{
    if(auto pChunk = allocateChunk(chunkIndex))
        return pChunk;
    if (!m_lruCachingReaderChunk) {
        qWarning() << "ERROR: No LRU chunk to free in allocateChunkExpireLRU.";
        return nullptr;
    }
    freeChunk(m_lruCachingReaderChunk);
    return allocateChunk(chunkIndex);
}
CachingReaderChunk* CachingReader::lookupChunk(SINT chunkIndex)
{
    // Defaults to nullptr if it's not in the hash.
    if(auto chunk = m_allocatedCachingReaderChunks.value(chunkIndex, nullptr)) {
        if(chunk->getState() == CachingReaderChunk::PENDING)
            return chunk;
        if(chunkIndex != chunk->getIndex()) {
            freeChunk(chunk);
            chunk = nullptr;
        }
        return chunk;
    }
    return nullptr;
}
void CachingReader::freshenChunk(CachingReaderChunk* pChunk)
{
    DEBUG_ASSERT(pChunk != nullptr);
    DEBUG_ASSERT(pChunk->getState() != CachingReaderChunk::PENDING);
    // Remove the chunk from the LRU list
    pChunk->removeFromList(&m_mruCachingReaderChunk, &m_lruCachingReaderChunk);
    // Adjust the least-recently-used item before inserting the
    // chunk as the new most-recently-used item.
    if (!m_lruCachingReaderChunk) {
        if (!m_mruCachingReaderChunk) {
            m_lruCachingReaderChunk = pChunk;
        } else {
            m_lruCachingReaderChunk = m_mruCachingReaderChunk;
        }
    }
    // Insert the chunk as the new most-recently-used item.
    pChunk->insertIntoListBefore(m_mruCachingReaderChunk);
    m_mruCachingReaderChunk = pChunk;
}
CachingReaderChunk* CachingReader::lookupChunkAndFreshen(SINT chunkIndex)
{
    auto pChunk = lookupChunk(chunkIndex);
    if (pChunk && (pChunk->getState() != CachingReaderChunk::PENDING))
        freshenChunk(pChunk);
    return pChunk;
}
void CachingReader::newTrack(TrackPointer pTrack)
{
    m_worker->newTrack(pTrack);
    m_worker->workReady();
}
void CachingReader::process()
{
    while (!m_readerStatusFIFO.empty()) {
        auto status = m_readerStatusFIFO.front();
        m_readerStatusFIFO.pop_front();
        if(auto pChunk = status.chunk) {
            // Take over control of the chunk from the worker.
            // This has to be done before freeing all chunks
            // after a new track has been loaded (see below)!
            pChunk->takeFromWorker();
            if (status.status != ReaderStatusUpdate::CHUNK_READ_SUCCESS) {
                // Discard chunks that are empty (EOF) or invalid
                freeChunk(pChunk);
            } else {
                // Insert or freshen the chunk in the MRU/LRU list after
                // obtaining ownership from the worker.
                freshenChunk(pChunk);
            }
        }
        if (status.status == ReaderStatusUpdate::TRACK_NOT_LOADED) {
            m_readerStatus = status.status;
        } else if (status.status == ReaderStatusUpdate::TRACK_LOADED) {
            m_readerStatus = status.status;
            // Reset the max. readable frame index
            m_maxReadableFrameIndex = status.maxReadableFrameIndex;
            // Free all chunks with sample data from a previous track
            freeAllChunks();
        }
        // Adjust the max. readable frame index
        if (m_readerStatus == ReaderStatusUpdate::TRACK_LOADED) {
            m_maxReadableFrameIndex = math_min(status.maxReadableFrameIndex, m_maxReadableFrameIndex);
        } else {
            m_maxReadableFrameIndex = mixxx::AudioSource::getMinFrameIndex();
        }
    }
}
SINT CachingReader::read(SINT sample, bool reverse, SINT numSamples, CSAMPLE* buffer)
{
    // Check for bad inputs
    DEBUG_ASSERT_AND_HANDLE(sample % CachingReaderChunk::kChannels == 0) {
        // This problem is easy to fix, but this type of call should be
        // complained about loudly.
        --sample;
    }
    DEBUG_ASSERT_AND_HANDLE(numSamples % CachingReaderChunk::kChannels == 0) {
        --numSamples;
    }
    if (numSamples < 0 || !buffer) {
        QString temp = QString("Sample = %1").arg(sample);
        qDebug() << "CachingReader::read() invalid arguments sample:" << sample
                 << "numSamples:" << numSamples << "buffer:" << buffer;
        return 0;
    }
    // If asked to read 0 samples, don't do anything. (this is a perfectly
    // reasonable request that happens sometimes. If no track is loaded, don't
    // do anything.
    if (numSamples == 0 || m_readerStatus != ReaderStatusUpdate::TRACK_LOADED) {
        return 0;
    }
    // Process messages from the reader thread.
    process();
    auto samplesRead = SINT{0};
    auto frameIndex = CachingReaderChunk::samples2frames(sample);
    auto numFrames = CachingReaderChunk::samples2frames(numSamples);

    // Fill the buffer up to the first readable sample with
    // silence. This may happen when the engine is in preroll,
    // i.e. if the frame index points a region before the first
    // track sample.
    if (mixxx::AudioSource::getMinFrameIndex() > frameIndex) {
        auto prerollFrames = math_min(numFrames,
                mixxx::AudioSource::getMinFrameIndex() - frameIndex);
        auto prerollSamples = CachingReaderChunk::frames2samples(prerollFrames);
        if (reverse) {
            SampleUtil::clear(&buffer[numSamples - prerollSamples], prerollSamples);
        } else {
            SampleUtil::clear(buffer, prerollSamples);
            buffer += prerollSamples;
        }
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
        DEBUG_ASSERT(mixxx::AudioSource::getMinFrameIndex() <= frameIndex);

        auto maxReadableFrameIndex = math_min(frameIndex + numFrames, m_maxReadableFrameIndex);
        if (maxReadableFrameIndex > frameIndex) {
            // The intersection between the readable samples from the track
            // and the requested samples is not empty, so start reading.

            auto firstCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(frameIndex);
            auto lastCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(maxReadableFrameIndex - 1);
            for (auto chunkIndex = firstCachingReaderChunkIndex; chunkIndex <= lastCachingReaderChunkIndex; ++chunkIndex) {

                auto pChunk = lookupChunkAndFreshen(chunkIndex);
                // If the chunk is not in cache, then we must return an error.
                if (!pChunk || (pChunk->getState() != CachingReaderChunk::READY)) {
                    Counter("CachingReader::read(): Failed to read chunk on cache miss")++;
                    // Exit the loop and fill the remaining buffer with silence
                    break;
                }
                // Please note that m_maxReadableFrameIndex might change with
                // every read operation! On a cache miss audio data will be
                // read from the audio source in lookupChunkAndFreshen() and
                // the max. readable frame index might be adjusted if decoding
                // errors occur.
                maxReadableFrameIndex = math_min(maxReadableFrameIndex, m_maxReadableFrameIndex);
                if (maxReadableFrameIndex <= frameIndex) {
                    // No more readable data available. Exit the loop and
                    // fill the remaining buffer with silence.
                    break;
                }
                DEBUG_ASSERT(0 < maxReadableFrameIndex);
                lastCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(maxReadableFrameIndex - 1);

                auto chunkFrameIndex = CachingReaderChunk::frameForIndex(chunkIndex);
                DEBUG_ASSERT(chunkFrameIndex <= frameIndex);
                DEBUG_ASSERT((chunkIndex == firstCachingReaderChunkIndex) ||
                        (chunkFrameIndex == frameIndex));
                auto chunkFrameOffset = frameIndex - chunkFrameIndex;
                DEBUG_ASSERT(chunkFrameOffset >= 0);
                auto chunkFrameCount = math_min(
                        pChunk->getFrameCount(),
                        maxReadableFrameIndex - chunkFrameIndex);
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

                if (reverse) {
                    pChunk->copySamplesReverse(&buffer[numSamples - samplesRead - samplesToCopy], chunkSampleOffset, samplesToCopy);
                } else {
                    pChunk->copySamples(buffer, chunkSampleOffset, samplesToCopy);
                    buffer += samplesToCopy;
                }
                samplesRead += samplesToCopy;
                frameIndex += framesToCopy;
            }
        }
    }
    // Finally fill the remaining buffer with silence.
    DEBUG_ASSERT(numSamples >= samplesRead);
    SampleUtil::clear(buffer, numSamples - samplesRead);
    return numSamples;
}
void CachingReader::hintAndMaybeWake(const HintVector& hintList)
{
    // If no file is loaded, skip.
    if (m_readerStatus != ReaderStatusUpdate::TRACK_LOADED)
        return;
    // For every chunk that the hints indicated, check if it is in the cache. If
    // any are not, then wake.
    auto shouldWake = false;
    for (auto hint : hintList) {
        // Copy, don't use reference.
        // Handle some special length values
        if (hint.length == 0) {
            hint.length = kDefaultHintSamples;
        } else if (hint.length == -1) {
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
        mixxx::AudioSource::clampFrameInterval(&minReadableFrameIndex, &maxReadableFrameIndex, m_maxReadableFrameIndex);
        if (minReadableFrameIndex >= maxReadableFrameIndex) {
            // skip empty frame interval silently
            continue;
        }
        auto firstCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(minReadableFrameIndex);
        auto lastCachingReaderChunkIndex = CachingReaderChunk::indexForFrame(maxReadableFrameIndex - 1);
        for (int chunkIndex = firstCachingReaderChunkIndex; chunkIndex <= lastCachingReaderChunkIndex; ++chunkIndex) {
            if(auto pChunk = lookupChunk(chunkIndex)) {
                if (pChunk->getState() == CachingReaderChunk::READY) {
                    // This will cause the chunk to be 'freshened' in the cache. The
                    // chunk will be moved to the end of the LRU list.
                    freshenChunk(pChunk);
                }
            }else{
                shouldWake = true;
                pChunk = allocateChunkExpireLRU(chunkIndex);
                if (pChunk == nullptr) {
                    qDebug() << "ERROR: Couldn't allocate spare CachingReaderChunk to make CachingReaderChunkReadRequest.";
                    continue;
                }
                // Do not insert the allocated chunk into the MRU/LRU list,
                // because it will be handed over to the worker immediately
                pChunk->giveToWorker();
                // qDebug() << "Requesting read of chunk" << current << "into" << pChunk;
                // qDebug() << "Requesting read into " << request.chunk->data;
                m_chunkReadRequestFIFO.push(pChunk);
                //qDebug() << "Checking chunk " << current << " shouldWake:" << shouldWake << " chunksToRead" << m_chunksToRead.size();
            }
        }
    }
    // If there are chunks to be read, wake up.
    if (shouldWake) {
        m_worker->workReady();
    }
}
