#include "sources/audiosource.h"

#include "util/sample.h"

namespace mixxx {

void AudioSource::clampFrameInterval(
        int64_t* pMinFrameIndexOfInterval,
        int64_t* pMaxFrameIndexOfInterval,
        int64_t maxFrameIndexOfAudioSource) {
    if (*pMinFrameIndexOfInterval < getMinFrameIndex()) {
        *pMinFrameIndexOfInterval = getMinFrameIndex();
    }
    if (*pMaxFrameIndexOfInterval > maxFrameIndexOfAudioSource) {
        *pMaxFrameIndexOfInterval = maxFrameIndexOfAudioSource;
    }
    if (*pMaxFrameIndexOfInterval < *pMinFrameIndexOfInterval) {
        *pMaxFrameIndexOfInterval = *pMinFrameIndexOfInterval;
    }
}
AudioSource::AudioSource(const QUrl& url)
        : UrlResource(url),
          AudioSignal(kSampleLayout),
          m_frameCount(kFrameCountDefault),
          m_bitrate(kBitrateDefault)
{ }
void AudioSource::setFrameCount(int64_t frameCount) {
    DEBUG_ASSERT(isValidFrameCount(frameCount));
    m_frameCount = frameCount;
}
void AudioSource::setBitrate(int64_t bitrate) {
    DEBUG_ASSERT(isValidBitrate(bitrate));
    m_bitrate = bitrate;
}
int64_t AudioSource::getSampleBufferSize(
        int64_t numberOfFrames,
        bool readStereoSamples) const
{
    if (readStereoSamples) {
        return numberOfFrames * kChannelCountStereo;
    } else {
        return frames2samples(numberOfFrames);
    }
}
int64_t AudioSource::readSampleFramesStereo(
        int64_t numberOfFrames,
        CSAMPLE* sampleBuffer,
        int64_t sampleBufferSize)
{
    DEBUG_ASSERT(getSampleBufferSize(numberOfFrames, true) <= sampleBufferSize);
    switch (getChannelCount()) {
        case 1: // mono channel
        {
            const int64_t readFrameCount = readSampleFrames(
                    numberOfFrames, sampleBuffer);
            SampleUtil::doubleMonoToDualMono(sampleBuffer, readFrameCount);
            return readFrameCount;
        }
        case 2: // stereo channel(s)
        {
            return readSampleFrames(numberOfFrames, sampleBuffer);
        }
        default: // multiple (3 or more) channels
        {
            const int64_t numberOfSamplesToRead = frames2samples(numberOfFrames);
            if (numberOfSamplesToRead <= sampleBufferSize) {
                // efficient in-place transformation
                const int64_t readFrameCount = readSampleFrames(
                        numberOfFrames, sampleBuffer);
                SampleUtil::copyMultiToStereo(sampleBuffer, sampleBuffer,
                        readFrameCount, getChannelCount());
                return readFrameCount;
            } else {
                // inefficient transformation through a temporary buffer
                qDebug() << "Performance warning:"
                        << "Allocating a temporary buffer of size"
                        << numberOfSamplesToRead << "for reading stereo samples."
                        << "The size of the provided sample buffer is"
                        << sampleBufferSize;
                SampleBuffer tempBuffer(numberOfSamplesToRead);
                const int64_t readFrameCount = readSampleFrames(
                        numberOfFrames, tempBuffer.data());
                SampleUtil::copyMultiToStereo(sampleBuffer, tempBuffer.data(),
                        readFrameCount, getChannelCount());
                return readFrameCount;
            }
        }
    }
}

bool AudioSource::verifyReadable() const {
    bool result = AudioSignal::verifyReadable();
    if (hasBitrate()) {
        DEBUG_ASSERT_AND_HANDLE(isValidBitrate(m_bitrate)) {
            qWarning() << "Invalid bitrate [kbps]:"
                    << getBitrate();
            // Don't set the result to false, because bitrate is only
            // an  informational property that does not effect the ability
            // to decode audio data!
        }
    }
    if (isEmpty()) {
        qWarning() << "AudioSource is empty and does not provide any audio data!";
        // Don't set the result to false, even if reading from an empty source
        // is pointless!
    }
    return result;
}

}
