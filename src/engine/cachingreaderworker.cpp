#include <QtDebug>
#include <QFileInfo>

#include "controlobject.h"
#include "controlobjectthread.h"

#include "engine/cachingreaderworker.h"
#include "trackinfoobject.h"
#include "soundsourceproxy.h"
#include "util/compatibility.h"
#include "util/event.h"
#include "util/math.h"


// One chunk should contain 1/2 - 1/4th of a second of audio.
// 8192 frames contain about 170 ms of audio at 48 kHz, which
// is well above (hopefully) the latencies people are seeing.
// At 10 ms latency one chunk is enough for 17 callbacks.
// Additionally the chunk size should be a power of 2 for
// easier memory alignment.
// TODO(XXX): The optimum value of the "constant" kFramesPerChunk
// depends on the properties of the AudioSource as the remarks
// above suggest!
const SINT CachingReaderWorker::kChunkChannels = Mixxx::AudioSource::kChannelCountStereo;
const SINT CachingReaderWorker::kFramesPerChunk = 8192; // ~ 170 ms at 48 kHz
const SINT CachingReaderWorker::kSamplesPerChunk = kFramesPerChunk * kChunkChannels;
CachingReaderWorker::CachingReaderWorker(const QString &group,
//        FIFO<ChunkReadRequest>* pChunkReadRequestFIFO,
        ChunkReadRequest &chunkReadRequest,
        FIFO<ReaderStatusUpdate>& readerStatusFIFO,
        Mixxx::AudioSourcePointer &pAudioSource)
        : EngineWorker()
        , m_group(group)
        , m_chunkReadRequest(chunkReadRequest)
        , m_readerStatusFIFO(readerStatusFIFO)
        , m_pAudioSource(pAudioSource){}
CachingReaderWorker::~CachingReaderWorker() {}
void CachingReaderWorker::run() {
      //qDebug() << "Processing ChunkReadRequest for" << chunk_number;
      // Initialize the output parameter
    ChunkReadRequest &request = m_chunkReadRequest;
    ReaderStatusUpdate update;
    update.chunk = request.chunk;
    update.chunk->frameCount = 0;
    const int chunk_number = request.chunk->chunk_number;
    do{
      if (!m_pAudioSource || chunk_number < 0) {
          update.status = CHUNK_READ_INVALID;
          break;
      }
      const SINT chunkFrameIndex = frameForChunk(chunk_number);
      if (!m_pAudioSource->isValidFrameIndex(chunkFrameIndex)) {
          // Frame index out of range
          qWarning() << "Invalid chunk seek position" << chunkFrameIndex;
          update.status = CHUNK_READ_INVALID;
          break;
      }
      const SINT seekFrameIndex = m_pAudioSource->seekSampleFrame(chunkFrameIndex);
      if (seekFrameIndex != chunkFrameIndex) {
          // Failed to seek to the requested index.
          // Corrupt file? -> Stop reading!
          qWarning() << "Failed to seek chunk position" << seekFrameIndex << "<>" << chunkFrameIndex;
          update.status = CHUNK_READ_INVALID;
          break;
      }
      const SINT framesRemaining = m_pAudioSource->getMaxFrameIndex() - seekFrameIndex;
      const SINT framesToRead    = math_min(kFramesPerChunk, framesRemaining);
      if (0 >= framesToRead) {
          // No more data available for reading
          update.status = CHUNK_READ_EOF;
          break;
      }
      const SINT framesRead =
              m_pAudioSource->readSampleFramesStereo(framesToRead, request.chunk->stereoSamples, kSamplesPerChunk);
      DEBUG_ASSERT(framesRead <= framesToRead);
      update.chunk->frameCount = framesRead;
      if (framesRead < framesToRead) {
          // Incomplete read! Corrupt file?
          qWarning() << "Incomplete chunk read @" << seekFrameIndex
                  << "[" << m_pAudioSource->getMinFrameIndex()
                  << "," << m_pAudioSource->getFrameCount()
                  << "]:" << framesRead << "<" << framesToRead;
          update.status = CHUNK_READ_PARTIAL;
      } else {update.status = CHUNK_READ_SUCCESS;}
    }while(0);
    m_readerStatusFIFO.write(&update,1);
}
