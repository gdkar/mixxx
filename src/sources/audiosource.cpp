#include "sources/audiosource.h"

#include "util/sample.h"

namespace Mixxx {
QUrl AudioSource::getUrl() const
{
    return m_url;
}
QString AudioSource::getUrlString() const
{
    return m_url.toString();
}
bool AudioSource::isLocalFile() const
{
    return getUrl().isLocalFile();
}
QString AudioSource::getLocalFileName() const
{
    return getUrl().toLocalFile();
}
void AudioSource::clampFrameInterval(
        SINT* pMinFrameIndexOfInterval,
        SINT* pMaxFrameIndexOfInterval,
        SINT maxFrameIndexOfAudioSource) {
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
        : AudioSignal(kSampleLayout),
          m_url(url),
          m_frameCount(kFrameCountDefault),
          m_bitrate(kBitrateDefault) {
}

void AudioSource::setFrameCount(SINT frameCount) {
    DEBUG_ASSERT(isValidFrameCount(frameCount));
    m_frameCount = frameCount;
}

void AudioSource::setBitrate(SINT bitrate) {
    DEBUG_ASSERT(isValidBitrate(bitrate));
    m_bitrate = bitrate;
}

SINT AudioSource::getSampleBufferSize(
        SINT numberOfFrames,
        bool readStereoSamples) const {
    if (readStereoSamples) {
        return numberOfFrames * kChannelCountStereo;
    } else {
        return frames2samples(numberOfFrames);
    }
}

SINT AudioSource::readSampleFramesStereo(
        SINT numberOfFrames,
        CSAMPLE* sampleBuffer,
        SINT sampleBufferSize) {
    DEBUG_ASSERT(getSampleBufferSize(numberOfFrames, true) <= sampleBufferSize);

    switch (getChannelCount()) {
        case 1: // mono channel
        {
            const SINT readFrameCount = readSampleFrames(
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
            const SINT numberOfSamplesToRead = frames2samples(numberOfFrames);
            if (numberOfSamplesToRead <= sampleBufferSize) {
                // efficient in-place transformation
                const SINT readFrameCount = readSampleFrames(
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
                const SINT readFrameCount = readSampleFrames(
                        numberOfFrames, tempBuffer.data());
                SampleUtil::copyMultiToStereo(sampleBuffer, tempBuffer.data(),
                        readFrameCount, getChannelCount());
                return readFrameCount;
            }
        }
    }
}
SINT AudioSource::getFrameCount() const
{
    return m_frameCount;
}
bool AudioSource::isEmpty() const
{
    return getFrameCount() > 0;
}
bool AudioSource::hasDuration() const
{
    return isValid();
}
SINT AudioSource::getDuration() const
{
    DEBUG_ASSERT(hasDuration()); // prevents division by zero
    return getFrameCount() / getSamplingRate();
}
/* static */ bool AudioSource::isValidBitrate(SINT bitrate)
{
    return bitrate > 0;
}
bool AudioSource::hasBitrate() const
{
    return m_bitrate > 0;
}
SINT AudioSource::getBitrate() const {
    DEBUG_ASSERT(hasBitrate()); // prevents reading an invalid bitrate
    return m_bitrate;
}

// Index of the first sample frame.
SINT AudioSource::getMinFrameIndex() {
    return kFrameIndexMin;
}

// Index of the sample frame following the last
// sample frame.
SINT AudioSource::getMaxFrameIndex() const {
    return getMinFrameIndex() + getFrameCount();
}

// The sample frame index is valid in the range
// [getMinFrameIndex(), getMaxFrameIndex()].
bool AudioSource::isValidFrameIndex(SINT frameIndex) const {
    return (getMinFrameIndex() <= frameIndex) &&
            (getMaxFrameIndex() >= frameIndex);
}
SINT AudioSource::skipSampleFrames(
        SINT numberOfFrames) {
    return readSampleFrames(numberOfFrames, static_cast<CSAMPLE*>(NULL));
}

SINT AudioSource::readSampleFrames(
        SINT numberOfFrames,
        SampleBuffer* pSampleBuffer) {
    if (pSampleBuffer) {
        DEBUG_ASSERT(frames2samples(numberOfFrames) <= pSampleBuffer->size());
        return readSampleFrames(numberOfFrames, pSampleBuffer->data());
    } else {
        return skipSampleFrames(numberOfFrames);
    }
}
SINT AudioSource::readSampleFramesStereo(
        SINT numberOfFrames,
        SampleBuffer* pSampleBuffer) {
    if (pSampleBuffer) {
        return readSampleFramesStereo(numberOfFrames,
                pSampleBuffer->data(), pSampleBuffer->size());
    } else {
        return skipSampleFrames(numberOfFrames);
    }
}
}
